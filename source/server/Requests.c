//
// Created by Marincia Cătălin on 21.12.2020.
//

#include "Requests.h"
#include "DatabaseInteraction.h"

#include <string.h>
#include <fcntl.h>
#include <limits.h>

u32 get_epoch_time(void) {
  return (u32)(time(NULL));
}

bool rq_register(sqlite3 *sqlite3_descriptor, JSON_Object *request_object, JSON_Object *response_object) {
  const char *username = json_object_dotget_string(request_object, "username");
  const char *password = json_object_dotget_string(request_object, "password");
  bool helper_bool;
  if (db_exists_user(sqlite3_descriptor, username, &helper_bool) == false) {
	json_object_set_string(response_object,
						   "message",
						   "[INT.ERROR] Cannot check the existence of the requested user in the database, that is making the creation of user impossible to succeed");
	return false;
  }
  if (helper_bool == true) {
	json_object_set_string(response_object, "message", "The user is already registered");
	return false;
  }
  if (db_add_user(sqlite3_descriptor, username, password) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot add the user to the database");
	return false;
  }
  json_object_set_string(response_object, "message", "The user was created");
  return true;
}

bool rq_login(sqlite3 *sqlite3_descriptor, JSON_Object *request_object) {
  bool helper_bool = false;
  const char *username = json_object_dotget_string(request_object, "username");
  const char *password = json_object_dotget_string(request_object, "password");
  CHECKRET(db_exist_credentials(sqlite3_descriptor, username, password, &helper_bool) == true, false, "Cannot check the existence of credentials")
  return helper_bool;
}

bool rq_allow_access_or_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
							 JSON_Object *response_object, bool allow_edit_mode) {
  bool helper_bool = false;
  const char *requester_username = json_object_dotget_string(request_object, "username");
  const char *other_username = json_object_dotget_string(request_object, "other_username");
  if (db_user_owns_repository(sqlite3_descriptor, repository_name, requester_username, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check if the user is the owner of the requested repository");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "You are not the creator of the repository so you cannot edit permissions");
	return false;
  }
  if (db_exists_user(sqlite3_descriptor, other_username, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of the requested username");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message",
						   "Cannot add permissions to the requested username since it does not exist in the database");
	return false;
  }
  if (db_allow_access(sqlite3_descriptor, repository_name, other_username) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot add access to the requested user");
	return false;
  }
  if (allow_edit_mode == true) {
	if (db_allow_edit(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_object, "message", "[INT.ERROR] Cannot add edit permissions to the requested user");
	  return false;
	}
  }
  json_object_set_string(response_object, "message", "Permissions to edit the repository have been granted");
  return true;
}

bool rq_block_access_or_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
							 JSON_Object *response_object, bool block_access_mode) {
  bool helper_bool = false;
  const char *requester_username = json_object_dotget_string(request_object, "username");
  const char *other_username = json_object_dotget_string(request_object, "other_username");
  if (db_user_owns_repository(sqlite3_descriptor, repository_name, requester_username, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check if the user is the owner of the requested repository");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "You are not the creator of the repository so you cannot edit permissions");
	return false;
  }
  if (strcmp(requester_username, other_username) == 0) {
	json_object_set_string(response_object, "message", "You cannot sabotage yourself by removing permissions to your own repositories");
	return false;
  }
  if (db_exists_user(sqlite3_descriptor, other_username, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of the requested username");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message",
						   "Cannot delete permissions of the requested username since it does not exist in the database");
	return false;
  }
  if (block_access_mode == true) {
	if (db_block_access(sqlite3_descriptor, repository_name, other_username) == false) {
	  json_object_set_string(response_object, "message", "[INT.ERROR] Cannot add edit access to the requested user");
	  return false;
	}
  }
  if (db_block_edit(sqlite3_descriptor, repository_name, other_username) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot add edit permissions to the requested user");
	return false;
  }
  json_object_set_string(response_object, "message", "Permissions to edit the repository have been revoked");
  return true;
}

bool rq_checkout_file(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
					  JSON_Object *response_object) {
  const char *filename = json_object_dotget_string(request_object, "filename");
  bool has_version = json_object_dotget_boolean(request_object, "has_version");
  u16 version = 0;
  char *file_content = (char *)(malloc(MB5));
  bzero(file_content, MB5);
  if (has_version == false) {
	if (db_get_latest_version(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_object, "message", "[INT.ERROR] Cannot get the current version");
	  free(file_content);
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_object, "version"));
  }
  if (db_get_file_content(sqlite3_descriptor, repository_name, version, filename, file_content) == false) {
	json_object_set_string(response_object, "message", "Cannot get the latest version of the file from repository");
	free(file_content);
	return false;
  }
  json_object_set_string(response_object, "filename", filename);
  json_object_set_string(response_object, "content", file_content);
  json_object_set_string(response_object, "message", "File at specific version has been successfully downloaded");
  free(file_content);
  return true;
}

bool rq_get_changelog(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
					  JSON_Object *response_object) {
  bool has_version = json_object_dotget_boolean(request_object, "has_version");
  u16 version = 0;
  i32 start;
  char *changelog = (char *)(malloc(MAX_CHANGELOG_LEN));
  bzero(changelog, MAX_CHANGELOG_LEN);
  if (has_version == false) {
	bool all_versions = json_object_dotget_boolean(request_object, "all_versions");
	if (db_get_latest_version(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_object, "message", "Cannot get the latest version for the repository");
	  free(changelog);
	  return false;
	}
	if (all_versions == true) {
	  start = 1;
	} else {
	  start = version;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_object, "version"));
	start = version;
  }
  JSON_Value *changelogs_value = json_value_init_array();
  JSON_Array *changelog_array = json_value_get_array(changelogs_value);
  for (i32 index = version; index >= start; --index) {
	if (db_get_changelog(sqlite3_descriptor, repository_name, index, changelog) == false) {
	  json_object_set_string(response_object, "message", "Cannot get the changelog for the repository at specific version");
	  free(changelog);
	  return false;
	}
	json_array_append_string(changelog_array, changelog);
  }
  json_object_set_value(response_object, "changelogs", changelogs_value);

  JSON_Value *unix_dates_value = json_value_init_array();
  JSON_Array *unix_dates_array = json_value_get_array(unix_dates_value);
  JSON_Value *versions_value = json_value_init_array();
  JSON_Array *versions_array = json_value_get_array(versions_value);
  u32 unix_date;
  for (i32 index = version; index >= start; --index) {
	if (db_get_push_unix_date(sqlite3_descriptor, repository_name, index, &unix_date) == false) {
	  json_object_set_string(response_object, "message", "Cannot get the unix date of push for the repository at specific version");
	  free(changelog);
	  return false;
	}
	json_array_append_number(unix_dates_array, unix_date);
	json_array_append_number(versions_array, index);
  }
  json_object_set_value(response_object, "dates", unix_dates_value);
  json_object_set_value(response_object, "versions", versions_value);
  json_object_set_string(response_object, "message", "Changelogs have been successfully queried");
  free(changelog);
  return true;
}

bool rq_list_of_remote_files(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
							 JSON_Object *response_object) {
  bool has_version = json_object_dotget_boolean(request_object, "has_version");
  JSON_Value *filename_value = json_value_init_array();
  JSON_Array *filename_array = json_value_get_array(filename_value);
  u16 version = 0;
  if (has_version == false) {
	if (db_get_latest_version(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_object, "message", "[INT.ERROR] Cannot get the current version");
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_object, "version"));
  }
  bool helper_bool = false;
  if (db_exists_repo_in_versions(sqlite3_descriptor, repository_name, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of repo in versions table");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "There is no version for the specified repository");
	return false;
  }
  if (db_exists_version(sqlite3_descriptor, repository_name, version, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of version in versions table");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "The version specified does not exist");
	return false;
  }
  if (db_get_list_of_files(sqlite3_descriptor, repository_name, version, filename_array) == false) {
	json_object_set_string(response_object, "message", "Cannot get the list of the files from the specific version");
	return false;
  }
  json_object_set_value(response_object, "files", filename_value);
  json_object_set_string(response_object, "message", "File list has been successfully queried");
  return true;
}

bool rq_main_distributor(sqlite3 *sqlite3_descriptor, const char *buffer, JSON_Value *response_value) {
  JSON_Object *response_object = json_value_get_object(response_value);
  JSON_Value *request_value = json_parse_string(buffer);
  CHECKRET(json_value_get_type(request_value) == JSONObject, false, "error at parsing string")
  JSON_Object *request_object = json_value_get_object(request_value);
  const char *message_type = json_object_dotget_string(request_object, "message_type");
  const char *repository_name = json_object_dotget_string(request_object, "repository_name");
  json_object_set_string(response_object, "repository_name", repository_name);
  bool is_public = false, has_access = false;
  if (strcmp(message_type, "register_request") != 0) {
	if (is_repo_public(sqlite3_descriptor, repository_name, &is_public) == false) {
	  json_object_set_string(response_object, "message", "Cannot check the access specifier of the repository");
	  return false;
	}
	if (strcmp(message_type, "is_repo_public_request") == 0) {
	  json_object_set_string(response_object, "message_type", "is_repo_public_response");
	  json_object_set_string(response_object, "message", "The visibility of repo was successfully queried");
	  json_object_set_boolean(response_object, "is_public", is_public);
	  return true;
	}
	if (is_public == false) {
	  if (rq_login(sqlite3_descriptor, request_object) == false) {
		json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
		return false;
	  }
	  if (strcmp(message_type, "push_request") != 0) {
		const char *username = json_object_dotget_string(request_object, "username");
		if (db_exists_access(sqlite3_descriptor, repository_name, username, &has_access) == false) {
		  json_object_set_string(response_object, "message", "Cannot check the access of username");
		  return false;
		}
		if (has_access == false) {
		  json_object_set_string(response_object,
								 "message",
								 "The repository is private/does not exist, you must wait for it to go public or have the owner give you access to it/be created");
		  return false;
		}
	  }
	}
  }
  if (strcmp(message_type, "register_request") == 0) {
	json_object_set_string(response_object, "message_type", "register_response");
	return rq_register(sqlite3_descriptor, request_object, response_object);
  }
  if (strcmp(message_type, "list_remote_files_request") == 0) {
	json_object_set_string(response_object, "message_type", "list_remote_files_response");
	return rq_list_of_remote_files(sqlite3_descriptor, repository_name, request_object, response_object);
  }
  if (strcmp(message_type, "allow_edit_request") == 0) {
	json_object_set_string(response_object, "message_type", "allow_edit_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_allow_access_or_edit(sqlite3_descriptor, repository_name, request_object, response_object, true);
  }
  if (strcmp(message_type, "checkout_file_request") == 0) {
	json_object_set_string(response_object, "message_type", "checkout_file_response");
	return rq_checkout_file(sqlite3_descriptor, repository_name, request_object, response_object);
  }
  if (strcmp(message_type, "diff_request") == 0) {
	json_object_set_string(response_object, "message_type", "diff_response");
	return rq_checkout_file(sqlite3_descriptor, repository_name, request_object, response_object);
  }
  if (strcmp(message_type, "get_changelog_request") == 0) {
	json_object_set_string(response_object, "message_type", "get_changelog_response");
	return rq_get_changelog(sqlite3_descriptor, repository_name, request_object, response_object);
  }
  if (strcmp(message_type, "block_edit_request") == 0) {
	json_object_set_string(response_object, "message_type", "dell_permission_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_block_access_or_edit(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }

  if (strcmp(message_type, "checkout_request") == 0) {
	json_object_set_string(response_object, "message_type", "checkout_response");
	return rq_pull_checkout_clone_or_difference(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }
  if (strcmp(message_type, "clone_request") == 0) {
	json_object_set_string(response_object, "message_type", "clone_response");
	return rq_pull_checkout_clone_or_difference(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }
  if (strcmp(message_type, "pull_request") == 0) {
	json_object_set_string(response_object, "message_type", "pull_response");
	return rq_pull_checkout_clone_or_difference(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }
  if (strcmp(message_type, "get_differences_request") == 0) {
	json_object_set_string(response_object, "message_type", "get_differences_response");
	return rq_pull_checkout_clone_or_difference(sqlite3_descriptor, repository_name, request_object, response_object, true);
  }
  if (strcmp(message_type, "push_request") == 0) {
	json_object_set_string(response_object, "message_type", "push_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_push(sqlite3_descriptor, repository_name, request_object, response_object);
  }
  if (strcmp(message_type, "allow_access_request") == 0) {
	json_object_set_string(response_object, "message_type", "add_access_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_allow_access_or_edit(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }
  if (strcmp(message_type, "block_access_request") == 0) {
	json_object_set_string(response_object, "message_type", "dell_access_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_block_access_or_edit(sqlite3_descriptor, repository_name, request_object, response_object, true);
  }
  if (strcmp(message_type, "make_repo_public_request") == 0) {
	json_object_set_string(response_object, "message_type", "make_repo_public_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_switch_repository_access(sqlite3_descriptor, repository_name, request_object, response_object, true);
  }
  if (strcmp(message_type, "make_repo_private_request") == 0) {
	json_object_set_string(response_object, "message_type", "make_repo_private_response");
	if (rq_login(sqlite3_descriptor, request_object) == false) {
	  json_object_set_string(response_object, "message", "Bad credentials, cannot resolve any operation");
	  return false;
	}
	return rq_switch_repository_access(sqlite3_descriptor, repository_name, request_object, response_object, false);
  }
  return true;
}

bool rq_pull_checkout_clone_or_difference(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
										  JSON_Object *response_object, bool difference_mode) {
  bool has_version = json_object_dotget_boolean(request_object, "has_version");
  u16 version = 0;
  if (has_version == false) {
	if (db_get_latest_version(sqlite3_descriptor, repository_name, &version) == false) {
	  json_object_set_string(response_object, "message", "[INT.ERROR] Cannot get the current version");
	  return false;
	}
  } else {
	version = (u16)(json_object_dotget_number(request_object, "version"));
  }
  bool helper_bool = false;
  if (db_exists_repo_in_repository(sqlite3_descriptor, repository_name, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of repo in REPOSITORY");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "The repository you are trying to operate on does not exist");
	return false;
  }
  if (db_exists_repo_in_versions(sqlite3_descriptor, repository_name, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of repo in VERSIONS");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "The repository you are trying to operate on does not have any versions");
	return false;
  }
  if (db_exists_repo_in_storage(sqlite3_descriptor, repository_name, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check the existence of repo in STORAGE");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "The repository you are trying to operate on does not have any files in STORAGE");
	return false;
  }
  json_object_set_number(response_object, "version", version);
  JSON_Value *file_names_value = json_value_init_array();
  JSON_Array *file_names_array = json_value_get_array(file_names_value);
  if (db_get_list_of_files(sqlite3_descriptor, repository_name, version, file_names_array) == false) {
	json_object_set_string(response_object, "message", "Cannot get the list of the files from the specific version");
	return false;
  }
  if (difference_mode == false) {
	json_object_set_value(response_object, "filenames", file_names_value);
	JSON_Value *file_contents_value = json_value_init_array();
	JSON_Array *file_contents_array = json_value_get_array(file_contents_value);
	char *file_content = (char *)(malloc(MB5));
	bzero(file_content, MB5);
	for (u32 index = 0; index < json_array_get_count(file_names_array); ++index) {
	  if (db_get_file_content(sqlite3_descriptor, repository_name, version, json_array_get_string(file_names_array, index), file_content) == false) {
		json_object_set_string(response_object, "message", "Cannot get the latest version of the file from repository");
		free(file_content);
		return false;
	  }
	  json_array_append_string(file_contents_array, file_content);
	}
	json_object_set_value(response_object, "filecontents", file_contents_value);
	free(file_content);
  } else {
	JSON_Value *difference_contents_value = json_value_init_array();
	JSON_Array *difference_contents_array = json_value_get_array(difference_contents_value);
	char *difference = (char *)(malloc(MB5));
	bzero(difference, MB5);
	for (u32 index = 0; index < json_array_get_count(file_names_array); ++index) {
	  if (db_get_file_diff(sqlite3_descriptor, repository_name, version, json_array_get_string(file_names_array, index), difference) == false) {
		json_object_set_string(response_object, "message", "Cannot get the latest version of the file difference from repository");
		free(difference);
		return false;
	  }
	  json_array_append_string(difference_contents_array, difference);
	}
	free(difference);
	JSON_Value *deleted_files_value = json_value_init_array();
	JSON_Array *deleted_files_array = json_value_get_array(deleted_files_value);
	if (db_get_list_of_deleted_files(sqlite3_descriptor, repository_name, version, deleted_files_array) == false) {
	  json_object_set_string(response_object, "message", "Cannot get the list of deleted files for the specified version");
	  return false;
	}
	for (u32 index = 0; index < json_array_get_count(deleted_files_array); ++index) {
	  json_array_append_string(file_names_array, json_array_get_string(deleted_files_array, index));
	  json_array_append_string(difference_contents_array, "file was deleted");
	}
	json_object_set_value(response_object, "differences", difference_contents_value);
	json_object_set_value(response_object, "filenames", file_names_value);
  }
  u32 unix_date;
  if (db_get_push_unix_date(sqlite3_descriptor, repository_name, version, &unix_date) == false) {
	json_object_set_string(response_object, "message", "Cannot get the unix date of push for the repository at specific version");
	return false;
  }
  json_object_dotset_number(response_object, "date", unix_date);
  json_object_set_string(response_object, "message", "Repository checkout|clone|pull|difference json has been successfully made");
  return true;
}

bool rq_push(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
			 JSON_Object *response_object) {
  const char *username = json_object_dotget_string(request_object, "username");
  u32 unix_date = get_epoch_time();
  bool helper_bool = false;
  if (db_exists_repo_in_repository(sqlite3_descriptor, repository_name, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "Cannot check the existence of repository in REPOSITORY");
	return false;
  }
  if (helper_bool == false) {
	// insert the repo
	if (db_insert_repo(sqlite3_descriptor, repository_name, username, unix_date) == false) {
	  json_object_set_string(response_object, "message", "Cannot add repository to REPOSITORY");
	  return false;
	}
	if (db_allow_edit(sqlite3_descriptor, repository_name, username) == false) {
	  json_object_set_string(response_object, "message", "Cannot add permissions over repository for user");
	  return false;
	}
	if (db_allow_access(sqlite3_descriptor, repository_name, username) == false) {
	  json_object_set_string(response_object, "message", "Cannot add access to repository for user");
	  return false;
	}
  } else {
	if (db_exists_edit(sqlite3_descriptor, repository_name, username, &helper_bool) == false) {
	  json_object_set_string(response_object, "message", "Cannot check the permissions of the user");
	  return false;
	}
	if (helper_bool == false) {
	  json_object_set_string(response_object, "message", "Access denied, you do not have permissions to create a new version/push");
	  return false;
	}
  }
  u16 new_version = 0;
  if (db_get_next_version(sqlite3_descriptor, repository_name, &new_version) == false) {
	json_object_set_string(response_object, "message", "Cannot figure out the next version number");
	return false;
  }
  const char *changelog = json_serialize_to_string(json_object_get_value(request_object, "changelog"));
  if (db_insert_version(sqlite3_descriptor, repository_name, username, new_version, unix_date, changelog) == false) {
	json_object_set_string(response_object, "message", "Cannot create new version entry");
	return false;
  }
  JSON_Array *file_names_array = json_object_get_array(request_object, "filenames");
  JSON_Array *file_contents_array = json_object_get_array(request_object, "filecontents");
  u32 count = json_array_get_count(file_names_array);
  char *difference = (char *)(malloc(MB5));
  bzero(difference, MB5);
  char *server_file_content = (char *)(malloc(MB5));
  bzero(difference, MB5);
  char *received_file_content = (char *)(malloc(MB5));
  bzero(received_file_content, MB5);
  char received_file_name[MAX_FILE_PATH_LEN];
  bzero(received_file_name, MAX_FILE_PATH_LEN);
  char path_file_version1[PATH_MAX];
  bzero(path_file_version1, PATH_MAX);
  char path_file_version2[PATH_MAX];
  bzero(path_file_version2, PATH_MAX);
  i32 fd_version1, fd_version2, pipes[2], nr_bytes_read;
  pid_t pid;
  // make sure to get the diff using unix_time variable, name of file, temp folder
  for (u32 index = 0; index < count; ++index) {
	strcpy(received_file_name, json_array_get_string(file_names_array, index));
	strcpy(received_file_content, json_array_get_string(file_contents_array, index));
	if (new_version == 1) {
	  strcpy(difference, "file was added");
	} else {
	  CHECKRET(db_exists_file(sqlite3_descriptor, repository_name, new_version - 1, received_file_name, &helper_bool) == true, false,
			   "Cannot check the existence of file")
	  if (helper_bool == false) {
		strcpy(difference, "file was added");
	  } else {
		CHECKRET(sprintf(path_file_version1, "/tmp/v%d_%s_%s", new_version - 1, username, received_file_name) > 0, false, "Error at 'sprintf()")
		CHECKRET(sprintf(path_file_version2, "/tmp/v%d_%s_%s", new_version, username, received_file_name) > 0, false, "Error at 'sprintf()")
		CHECKRET(db_get_file_content(sqlite3_descriptor, repository_name, new_version - 1, received_file_name, server_file_content) == true, false,
				 "Cannot get the content of the file that is on the server")
		fd_version1 = open(path_file_version1, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		fd_version2 = open(path_file_version2, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		CHECKRET(fd_version1 != -1, false, "Error at opening file descriptor 1")
		CHECKRET(fd_version2 != -1, false, "Error at opening file descriptor 2")
		CHECKRET(write(fd_version1, server_file_content, strlen(server_file_content)) == strlen(server_file_content), false, "Error, partial write")
		CHECKRET(write(fd_version2, received_file_content, strlen(received_file_content)) == strlen(received_file_content), false,
				 "Error, partial write")
		CHECKRET(pipe(pipes) != -1, false, "Error at pipe()")
		CHECKRET((pid = fork()) != -1, false, "Error at fork()")
		if (pid == 0) {
		  // child
		  CHECKRET(close(pipes[READ_END]) == 0, false, "Error at close() READ_END")
		  CHECKRET(dup2(pipes[WRITE_END], STDOUT_FILENO) != -1, false, "Error at dup2()")
		  CHECKEXIT(execlp("diff", "diff", "-u", path_file_version1, path_file_version2, NULL) != -1, "Error at execlp()")
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
	  strcpy(difference, "unchanged");
	}
	if (db_insert_file(sqlite3_descriptor, repository_name, new_version, received_file_name, received_file_content, difference) == false) {
	  json_object_set_string(response_object, "message", "Cannot add file entry to the storage database");
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
	if (db_get_list_of_files(sqlite3_descriptor, repository_name, new_version - 1, previous_version_files_array) == true) {
	  count = json_array_get_count(previous_version_files_array);
	  for (u32 index = 0; index < count; ++index) {

		if (db_exists_file(sqlite3_descriptor, repository_name, new_version, json_array_get_string(previous_version_files_array,
																								   index), &helper_bool) == false) {
		  json_object_set_string(response_object, "message", "Cannot for the files that were deleted in the newer "
															 "version");

		  return false;
		}
		if (helper_bool == false) {
		  CHECKRET(db_mark_as_deleted(sqlite3_descriptor, repository_name, new_version,
									  json_array_get_string(previous_version_files_array, index)) == true, false,
				   "Cannot add filename to the DELETIONS database")
		}
	  }
	}
  }
  json_object_set_string(response_object, "message", "Repository push operation has been successfully made");
  return true;
}

bool rq_switch_repository_access(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
								 JSON_Object *response_object, bool make_public) {
  bool helper_bool = false;
  const char *username = json_object_dotget_string(request_object, "username");
  if (db_user_owns_repository(sqlite3_descriptor, repository_name, username, &helper_bool) == false) {
	json_object_set_string(response_object, "message", "[INT.ERROR] Cannot check if the user is the owner of the requested repository");
	return false;
  }
  if (helper_bool == false) {
	json_object_set_string(response_object, "message", "You are not the creator of the repository so you cannot edit the access of it");
	return false;
  }
  if (db_switch_repository_access(sqlite3_descriptor, repository_name, make_public) == false) {
	json_object_set_string(response_object, "message", "Cannot change the access of the repository");
	return false;
  }
  if (make_public == true) {
	json_object_set_string(response_object, "message", "Repository made public with success");
  } else {
	json_object_set_string(response_object, "message", "Repository made private with success");
  }
  return true;
}
