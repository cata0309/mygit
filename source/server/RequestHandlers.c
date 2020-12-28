//
// Created by Marincia Cătălin on 21.12.2020.
//

#include "RequestHandlers.h"
#include "DbOperations.h"

#include <string.h>
#include <fcntl.h>

u32 get_current_time(void) {
  return (u32)(time(NULL));
}

bool handler_add_user_request(sqlite3 *sqlite3_descriptor,
							  JSON_Object *request_root_object,
							  JSON_Object *response_root_object) {
  const char *username = json_object_dotget_string(request_root_object, "username");
  const char *password = json_object_dotget_string(request_root_object, "password");
  bool b_value;
  if (user_exists(sqlite3_descriptor, username, &b_value) == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check the existence of the requested user in the "
						   "database, that is making the creation of user impossible to succeed");
	return false;
  }
  if (b_value == true) {
	json_object_set_string(response_root_object, "message", "The user is already registered");
	return false;
  }
  if (add_user(sqlite3_descriptor, username, password) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot add the user to the database");
	return false;
  }
  json_object_set_string(response_root_object, "message", "The user was created");
  return true;
}

bool embedded_login_request(sqlite3 *sqlite3_descriptor, JSON_Object *request_root_object) {
  bool local_bool = false;
  const char *username = json_object_dotget_string(request_root_object, "username");
  const char *password = json_object_dotget_string(request_root_object, "password");
  CHECKRET(credentials_exist(sqlite3_descriptor, username, password, &local_bool) == true, false,
		   "Cannot check the existence of credentials")
  return local_bool;
}

bool handler_add_permission_or_access_request(sqlite3 *sqlite3_descriptor,
											  const char *repository_name,
											  JSON_Object *request_root_object,
											  JSON_Object *response_root_object,
											  bool add_access_mode) {
  bool local_bool = false;
  const char *requester_username = json_object_dotget_string(request_root_object, "username");
  const char *other_username = json_object_dotget_string(request_root_object, "other_username");
  if (is_the_user_the_creator_of_the_repo(sqlite3_descriptor, repository_name, requester_username, &local_bool) == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check if the user is the owner of the requested "
						   "repository");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "You are not the creator of the repository so you cannot "
						   "edit permissions");
	return false;
  }
  if (user_exists(sqlite3_descriptor, other_username, &local_bool) == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check the existence of the requested username");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "Cannot add permissions to the requested username since it"
						   " does not exist in the database");
	return false;
  }
  if (add_access_mode == true) {
	if (add_access(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot add access to the requested user");
	  return false;
	}
  } else {
	if (add_permission(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_root_object,
							 "message",
							 "[INT.ERROR] Cannot add edit permissions to the requested user");
	  return false;
	}
  }
  json_object_set_string(response_root_object, "message", "Permissions to edit the repository have been granted");
  return true;
}

bool handler_del_permission_or_access_request(sqlite3 *sqlite3_descriptor,
											  const char *repository_name,
											  JSON_Object *request_root_object,
											  JSON_Object *response_root_object,
											  bool delete_access_mode) {
  bool local_bool = false;
  const char *requester_username = json_object_dotget_string(request_root_object, "username");
  const char *other_username = json_object_dotget_string(request_root_object, "other_username");
  if (is_the_user_the_creator_of_the_repo(sqlite3_descriptor, repository_name, requester_username, &local_bool) == false) {

	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check if the user is the owner of the requested "
						   "repository");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "You are not the creator of the repository so you cannot "
						   "edit permissions");
	return false;
  }
  if (strcmp(requester_username, other_username) == 0) {
	json_object_set_string(response_root_object,
						   "message",
						   "You cannot sabotage yourself by removing permissions to your own repositories");
	return false;
  }
  if (user_exists(sqlite3_descriptor, other_username, &local_bool) == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check the existence of the requested username");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "Cannot delete permissions of the requested username since it"
						   " does not exist in the database");
	return false;
  }
  if (delete_access_mode == true) {
	if (del_access(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_root_object,
							 "message",
							 "[INT.ERROR] Cannot add edit access to the requested user");
	  return false;
	}
  } else {
	if (del_permission(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_root_object,
							 "message",
							 "[INT.ERROR] Cannot add edit permissions to the requested user");
	  return false;
	}
  }
  json_object_set_string(response_root_object, "message", "Permissions to edit the repository have been revoked");
  return true;
}

bool handler_checkout_file_request(sqlite3 *sqlite3_descriptor,
								   const char *repository_name,
								   JSON_Object *request_root_object,
								   JSON_Object *response_root_object) {
  const char *filename = json_object_dotget_string(request_root_object, "filename");
  bool has_version = json_object_dotget_boolean(request_root_object, "has_version");
  u16 version = 0;
  char *file_content = (char *)(malloc(MB5));
  if (has_version == false) {
	if (get_current_version_number(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot get the current version");
	  free(file_content);
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_root_object, "version"));
  }
  if (get_file_content(sqlite3_descriptor, repository_name, version, filename, file_content) == false) {
	json_object_set_string(response_root_object, "message", "Cannot get the latest version of the file from "
															"repository");
	free(file_content);
	return false;
  }
  json_object_set_string(response_root_object, "filename", filename);
  json_object_set_string(response_root_object, "content", file_content);
  json_object_set_string(response_root_object, "message", "File at specific version has been successfully "
														  "downloaded");
  free(file_content);
  return true;
}

bool handler_get_changelog_request(sqlite3 *sqlite3_descriptor,
								   const char *repository_name,
								   JSON_Object *request_root_object,
								   JSON_Object *response_root_object) {
  bool has_version = json_object_dotget_boolean(request_root_object, "has_version");
  u16 version = 0;
  i32 start;
  char *changelog = (char *)(malloc(MAX_CHANGELOG_LEN));
  if (has_version == false) {
	bool all_versions = json_object_dotget_boolean(request_root_object, "all_versions");
	if (get_current_version_number(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot get the latest version for the "
															  "repository");
	  free(changelog);
	  return false;
	}
	if (all_versions == true) {
	  start = 1;
	} else {
	  start = version;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_root_object, "version"));
	start = version;
  }
  JSON_Value *changelogs_value = json_value_init_array();
  JSON_Array *changelog_array = json_value_get_array(changelogs_value);
  for (i32 index = version; index >= start; --index) {
	if (get_changelog_version(sqlite3_descriptor, repository_name, index, changelog) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot get the changelog for the repository at "
															  "specific version");
	  free(changelog);
	  return false;
	}
	json_array_append_string(changelog_array, changelog);
  }
  json_object_set_value(response_root_object, "changelogs", changelogs_value);

  JSON_Value *unix_dates_value = json_value_init_array();
  JSON_Array *unix_dates_array = json_value_get_array(unix_dates_value);
  u32 unix_date;
  for (i32 index = version; index >= start; --index) {
	if (get_push_time_version(sqlite3_descriptor, repository_name, index, &unix_date) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot get the unix date of push for the repository at "
															  "specific version");
	  free(changelog);
	  return false;
	}
	json_array_append_number(unix_dates_array, unix_date);
  }
  json_object_set_value(response_root_object, "dates", unix_dates_value);
  json_object_set_string(response_root_object, "message", "Changelogs have been successfully queried");
  free(changelog);
  return true;
}

bool handler_ls_remote_files_request(sqlite3 *sqlite3_descriptor,
									 const char *repository_name,
									 JSON_Object *request_root_object,
									 JSON_Object *response_root_object) {
  bool has_version = json_object_dotget_boolean(request_root_object, "has_version");
  JSON_Value *filename_value = json_value_init_array();
  JSON_Array *filename_array = json_value_get_array(filename_value);
  u16 version = 0;
  if (has_version == false) {
	if (get_current_version_number(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot get the current version");
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_root_object, "version"));
  }
  bool local_bool = false;
  if (repo_exists_in_VERSIONS(sqlite3_descriptor, repository_name, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot check the existence of repo in versions "
															"table");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object, "message", "There is no version for the specified repository");
	return false;
  }
  if (version_exists(sqlite3_descriptor, repository_name, version, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot check the existence of version in "
															"versions table");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object, "message", "The version specified does not exist");
	return false;
  }
  if (get_ls_remote_files_version(sqlite3_descriptor, repository_name, version, filename_array) == false) {
	json_object_set_string(response_root_object, "message", "Cannot get the list of the files from the specific "
															"version");
	return false;
  }
  json_object_set_value(response_root_object, "files", filename_value);
  json_object_set_string(response_root_object, "message", "File list has been successfully queried");
  return true;
}

bool handler_requests(sqlite3 *sqlite3_descriptor, const char *buffer, JSON_Value *response_root_value) {
  JSON_Object *response_root_object = json_value_get_object(response_root_value);
  JSON_Value *request_root_value = json_parse_string(buffer);
  CHECKRET(json_value_get_type(request_root_value) == JSONObject, false, "error at parsing string")
  JSON_Object *request_root_object = json_value_get_object(request_root_value);
  const char *message_type = json_object_dotget_string(request_root_object, "message_type");
  const char *repository_name = json_object_dotget_string(request_root_object, "repository_name");
  json_object_set_string(response_root_object, "repository_name", repository_name);
  bool is_public, has_access;
  if (strcmp(message_type, "add_user_request") != 0) {
	if (is_repo_public(sqlite3_descriptor, repository_name, &is_public) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot check the access specifier of the repository");
	  return false;
	}
	if (strcmp(message_type, "is_repo_public_request") == 0) {
	  json_object_set_string(response_root_object, "message_type", "is_repo_public_response");
	  json_object_set_string(response_root_object, "message", "The visibility of repo was successfully queried");
	  json_object_set_boolean(request_root_object, "is_public", is_public);
	  return true;
	}
	if (is_public == false) {
	  if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
		json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
		return false;
	  }
	  if (strcmp(message_type, "push_request") != 0) {
		const char *username = json_object_dotget_string(request_root_object, "username");
		if (access_exists(sqlite3_descriptor, repository_name, username, &has_access) == false) {
		  json_object_set_string(response_root_object, "message", "Cannot check the access of username");
		  return false;
		}
		if (has_access == false) {
		  json_object_set_string(response_root_object,
								 "message",
								 "The repository is private/does not exist, you must wait for it to go "
								 "public or have the owner give you access to it/be created");
		  return false;
		}
	  }
	}
  }
  if (strcmp(message_type, "add_user_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "add_user_response");
	return handler_add_user_request(sqlite3_descriptor, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "add_permission_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "add_permission_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_add_permission_or_access_request(sqlite3_descriptor,
													repository_name,
													request_root_object,
													response_root_object, false);
  }
  if (strcmp(message_type, "checkout_file_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "checkout_file_response");
	return handler_checkout_file_request(sqlite3_descriptor, repository_name, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "diff_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "diff_response");
	return handler_checkout_file_request(sqlite3_descriptor, repository_name, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "get_changelog_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "get_changelog_response");
	return handler_get_changelog_request(sqlite3_descriptor, repository_name, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "del_permission_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "dell_permission_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_del_permission_or_access_request(sqlite3_descriptor,
													repository_name,
													request_root_object,
													response_root_object, false);
  }
  if (strcmp(message_type, "ls_remote_files_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "ls_remote_files_response");
	return handler_ls_remote_files_request(sqlite3_descriptor, repository_name, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "checkout_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "checkout_response");
	return handler_checkout_or_differences_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object,
												   false);
  }
  if (strcmp(message_type, "clone_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "clone_response");
	return handler_checkout_or_differences_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object,
												   false);
  }
  if (strcmp(message_type, "pull_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "pull_response");
	return handler_checkout_or_differences_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object,
												   false);
  }
  if (strcmp(message_type, "get_differences_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "get_differences_response");
	return handler_checkout_or_differences_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object,
												   true);
  }
  if (strcmp(message_type, "push_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "push_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_push_request(sqlite3_descriptor, repository_name, request_root_object, response_root_object);
  }
  if (strcmp(message_type, "add_access_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "add_access_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_add_permission_or_access_request(sqlite3_descriptor,
													repository_name,
													request_root_object,
													response_root_object, true);
  }
  if (strcmp(message_type, "del_access_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "dell_access_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_del_permission_or_access_request(sqlite3_descriptor,
													repository_name,
													request_root_object,
													response_root_object, true);
  }
  if (strcmp(message_type, "make_repo_public_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "make_repo_public_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_make_repo_switch_access_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object, true);
  }
  if (strcmp(message_type, "make_repo_private_request") == 0) {
	json_object_set_string(response_root_object, "message_type", "make_repo_private_response");
	if (embedded_login_request(sqlite3_descriptor, request_root_object) == false) {
	  json_object_set_string(response_root_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return handler_make_repo_switch_access_request(sqlite3_descriptor,
												   repository_name,
												   request_root_object,
												   response_root_object, false);
  }
  return true;
}

bool handler_checkout_or_differences_request(sqlite3 *sqlite3_descriptor,
											 const char *repository_name,
											 JSON_Object *request_root_object,
											 JSON_Object *response_root_object,
											 bool is_difference) {
  bool has_version = json_object_dotget_boolean(request_root_object, "has_version");
  u16 version = 0;
  if (has_version == false) {
	if (get_current_version_number(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot get the current version");
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_root_object, "version"));
  }
  bool local_bool = false;
  if (repo_exists_in_REPOSITORY(sqlite3_descriptor, repository_name, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot check the existence of repo in REPOSITORY");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object, "message", "The repository you are trying to operate on does not exist");
	return false;
  }
  if (repo_exists_in_VERSIONS(sqlite3_descriptor, repository_name, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot check the existence of repo in VERSIONS");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object, "message", "The repository you are trying to operate on does not have any "
															"versions");
	return false;
  }
  if (repo_exists_in_STORAGE(sqlite3_descriptor, repository_name, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "[INT.ERROR] Cannot check the existence of repo in STORAGE");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object, "message", "The repository you are trying to operate on does not have any "
															"files in STORAGE");
	return false;
  }
  json_object_set_number(response_root_object, "version", version);
  JSON_Value *file_names_value = json_value_init_array();
  JSON_Array *file_names_array = json_value_get_array(file_names_value);
  if (get_ls_remote_files_version(sqlite3_descriptor, repository_name, version, file_names_array) == false) {
	json_object_set_string(response_root_object, "message", "Cannot get the list of the files from the specific "
															"version");
	return false;
  }
  if (is_difference == false) {
	json_object_set_value(response_root_object, "filenames", file_names_value);
	JSON_Value *file_contents_value = json_value_init_array();
	JSON_Array *file_contents_array = json_value_get_array(file_contents_value);
	char *file_content = (char *)(malloc(MB5));
	for (u32 index = 0; index < json_array_get_count(file_names_array); ++index) {
	  if (get_file_content(sqlite3_descriptor,
						   repository_name,
						   version,
						   json_array_get_string(file_names_array, index),
						   file_content)
		  == false) {
		json_object_set_string(response_root_object, "message", "Cannot get the latest version of the file from "
																"repository");
		free(file_content);
		return false;
	  }
	  json_array_append_string(file_contents_array, file_content);
	}
	json_object_set_value(response_root_object, "filecontents", file_contents_value);
	free(file_content);
  } else {
	JSON_Value *difference_contents_value = json_value_init_array();
	JSON_Array *difference_contents_array = json_value_get_array(difference_contents_value);
	char *difference = (char *)(malloc(MB5));
	for (u32 index = 0; index < json_array_get_count(file_names_array); ++index) {
	  if (get_file_difference(sqlite3_descriptor,
							  repository_name,
							  version,
							  json_array_get_string(file_names_array, index),
							  difference)
		  == false) {
		json_object_set_string(response_root_object, "message", "Cannot get the latest version of the file difference from "
																"repository");
		free(difference);
		return false;
	  }
	  json_array_append_string(difference_contents_array, difference);
	}
	free(difference);

	JSON_Value *deleted_files_value = json_value_init_array();
	JSON_Array *deleted_files_array = json_value_get_array(deleted_files_value);
	if (get_ls_deleted_files_version(sqlite3_descriptor, repository_name, version, deleted_files_array) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot get the list of deleted files for the specified "
															  "version");
	  return false;
	}
	for (u32 index = 0; index < json_array_get_count(deleted_files_array); ++index) {
	  json_array_append_string(file_names_array, json_array_get_string(deleted_files_array, index));
	  json_array_append_string(difference_contents_array, "the file was deleted");
	}
	json_object_set_value(response_root_object, "differences", difference_contents_value);
	json_object_set_value(response_root_object, "filenames", file_names_value);
  }
  u32 unix_date;
  if (get_push_time_version(sqlite3_descriptor, repository_name, version, &unix_date) == false) {
	json_object_set_string(response_root_object, "message", "Cannot get the unix date of push for the repository at "
															"specific version");
	return false;
  }
  json_object_dotset_number(response_root_object, "date", unix_date);
  json_object_set_string(response_root_object, "message", "Repository checkout|clone|pull|difference json has been "
														  "successfully made");
  return true;
}

bool handler_push_request(sqlite3 *sqlite3_descriptor,
						  const char *repository_name,
						  JSON_Object *request_root_object,
						  JSON_Object *response_root_object) {
  const char *username = json_object_dotget_string(request_root_object, "username");
  u32 unix_date = get_current_time();
  bool local_bool = false;
  if (repo_exists_in_REPOSITORY(sqlite3_descriptor, repository_name, &local_bool) == false) {
	json_object_set_string(response_root_object, "message", "Cannot check the existence of repository in REPOSITORY");
	return false;
  }
  if (local_bool == false) {
	// insert the repo
	if (add_repo(sqlite3_descriptor, repository_name, username, unix_date) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot add repository to REPOSITORY");
	  return false;
	}
	if (add_permission(sqlite3_descriptor, repository_name, username) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot add permissions over repository for user");
	  return false;
	}
	if (add_access(sqlite3_descriptor, repository_name, username) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot add access to repository for user");
	  return false;
	}
  } else {
	if (permission_exists(sqlite3_descriptor, repository_name, username, &local_bool) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot check the permissions of the user");
	  return false;
	}
	if (local_bool == false) {
	  json_object_set_string(response_root_object, "message", "Access denied, you do not have permissions to create a new "
															  "version/push");
	  return false;
	}
  }
  u16 new_version = 0;
  if (get_next_version_number(sqlite3_descriptor, repository_name, &new_version) == false) {
	json_object_set_string(response_root_object, "message", "Cannot figure out the next version number");
	return false;
  }
  const char *changelog = json_object_dotget_string(request_root_object, "changelog");
  if (add_version(sqlite3_descriptor, repository_name, username, new_version, unix_date, changelog) == false) {
	json_object_set_string(response_root_object, "message", "Cannot create new version entry");
	return false;
  }
  JSON_Array *file_names_array = json_object_get_array(request_root_object, "filenames");
  JSON_Array *file_contents_array = json_object_get_array(request_root_object, "filecontents");
  u32 count = json_array_get_count(file_names_array);
  char *difference = (char *)(malloc(MB5));
  char *server_file_content = (char *)(malloc(MB5));
  char *received_file_content = (char *)(malloc(MB5));
  char received_file_name[MAX_FILE_PATH_LEN];
  char path_file_version1[MAX_FILE_PATH_LEN];
  char path_file_version2[MAX_FILE_PATH_LEN];
  i32 fd_version1;
  i32 fd_version2;
  i32 pipes[2];
  i32 nr_bytes_read;
  pid_t pid;
  // make sure to get the diff using unix_time variable, name of file, temp folder
  for (u32 index = 0; index < count; ++index) {
	strcpy(received_file_name, json_array_get_string(file_names_array, index));
	strcpy(received_file_content, json_array_get_string(file_contents_array, index));
	if (new_version == 1) {
	  strcpy(difference, "file was added");
	} else {
	  CHECKRET(file_exists(sqlite3_descriptor, repository_name, new_version - 1, received_file_name, &local_bool) == true,
			   false, "Cannot check the existence of file")
	  if (local_bool == false) {
		strcpy(difference, "file was added");
	  } else {
		CHECKRET(sprintf(path_file_version1, "/tmp/v%d_%s_%s", new_version - 1, username, received_file_name) > 0,
				 false,
				 "Error at 'sprintf()")
		CHECKRET(sprintf(path_file_version2, "/tmp/v%d_%s_%s", new_version, username, received_file_name) > 0,
				 false,
				 "Error at 'sprintf()")
		CHECKRET(
			get_file_content(sqlite3_descriptor, repository_name, new_version - 1, received_file_name, server_file_content)
				== true, false, "Cannot get the content of the file that is on the server")
		fd_version1 = open(path_file_version1, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		fd_version2 = open(path_file_version2, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		CHECKRET(fd_version1 != -1, false, "Error at opening file descriptor 1")
		CHECKRET(fd_version2 != -1, false, "Error at opening file descriptor 2")
		CHECKRET(write(fd_version1, server_file_content, strlen(server_file_content)) == strlen(server_file_content), false,
				 "Error, partial write")
		CHECKRET(write(fd_version2, received_file_content, strlen(received_file_content)) == strlen(received_file_content),
				 false,
				 "Error, partial write")
		CHECKRET(pipe(pipes) != -1, false, "Error at pipe()")
		CHECKRET((pid = fork()) != -1, false, "Error at fork()")
		if (pid == 0) {
		  // child
		  CHECKRET(close(pipes[READ_END]) == 0, false, "Error at close() READ_END")
		  CHECKRET(dup2(pipes[WRITE_END], STDOUT_FILENO) != -1, false, "Error at dup2()")
		  CHECKEXIT(execlp("diff", "diff", "-u", path_file_version1, path_file_version2, NULL) != -1, false,
					"Error at execlp()")
		  // no need for exit since it is used exec
		}
		// parent
		CHECKRET(close(pipes[WRITE_END]) == 0, false, "Error at close() WRITE_END")
		CHECKRET((nr_bytes_read = read(pipes[READ_END], difference, MB5)) != -1, false, "Error at read()")
		difference[nr_bytes_read] = '\0';
		CHECKRET(close(pipes[READ_END]) == 0, false, "Error at close() READ_END")
		CHECKRET(close(fd_version1) == 0, false, "Error at close() fd_version1")
		CHECKRET(close(fd_version2) == 0, false, "Error at close() fd_version2")
	  }
	}
	if (strlen(difference) == 0) {
	  strcpy(difference, "nothing changed");
	}
	if (add_file(sqlite3_descriptor,
				 repository_name,
				 new_version,
				 received_file_name,
				 received_file_content, difference) == false) {
	  json_object_set_string(response_root_object, "message", "Cannot add file entry to the storage database");
	  free(difference);
	  free(server_file_content);
	  return false;
	}
  }
  free(difference);
  free(server_file_content);
  free(received_file_content);
  if (new_version != 1) {
	JSON_Value *previous_version_files_value = json_value_init_array();
	JSON_Array *previous_version_files_array = json_value_get_array(previous_version_files_value);
	if (get_ls_remote_files_version(sqlite3_descriptor, repository_name, new_version - 1, previous_version_files_array)
		== true) {
	  count = json_array_get_count(previous_version_files_array);
	  for (u32 index = 0; index < count; ++index) {

		if (file_exists(sqlite3_descriptor, repository_name, new_version, json_array_get_string(previous_version_files_array,
																								index), &local_bool)
			== false) {
		  json_object_set_string(response_root_object, "message", "Cannot for the files that were deleted in the newer "
																  "version");

		  return false;
		}
		if (local_bool == false) {
		  CHECKRET(add_deleted_filename(sqlite3_descriptor, repository_name, new_version,
										json_array_get_string(previous_version_files_array, index)) == true, false,
				   "Cannot add filename to the DELETIONS database")
		}
	  }
	}
  }
  json_object_set_string(response_root_object, "message", "Repository push operation has been successfully made");
  return true;
}

bool handler_make_repo_switch_access_request(sqlite3 *sqlite3_descriptor,
											 const char *repository_name,
											 JSON_Object *request_root_object,
											 JSON_Object *response_root_object,
											 bool make_public) {
  bool local_bool = false;
  const char *username = json_object_dotget_string(request_root_object, "username");
  if (is_the_user_the_creator_of_the_repo(sqlite3_descriptor, repository_name, username, &local_bool) == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "[INT.ERROR] Cannot check if the user is the owner of the requested "
						   "repository");
	return false;
  }
  if (local_bool == false) {
	json_object_set_string(response_root_object,
						   "message",
						   "You are not the creator of the repository so you cannot "
						   "edit the access of it");
	return false;
  }
  if (make_public == true) {
	if (make_repo_public(sqlite3_descriptor, repository_name) == false) {
	  json_object_set_string(response_root_object,
							 "message",
							 "Cannot make repository public");
	  return false;
	}
	json_object_set_string(response_root_object,
						   "message",
						   "Repository made public with success");
  } else {
	if (make_repo_private(sqlite3_descriptor, repository_name) == false) {
	  json_object_set_string(response_root_object,
							 "message",
							 "Cannot make repository private");
	  return false;
	}
	json_object_set_string(response_root_object,
						   "message",
						   "Repository made private with success");
  }
  return true;
}
