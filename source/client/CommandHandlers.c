//
// Created by Marincia Cătălin on 23.12.2020.
//

#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <fcntl.h>
#include <dirent.h>
#include "CommandHandlers.h"
#include "../common/Transfer.h"
void get_password_from_stdin(bool confirm, char *password) {
  fflush(stdout);
  if (confirm == true) {
	printf("Confirm password: ");
  } else {
	printf("Password: ");
  }
  fflush(stdout);
  static struct termios old_settings;
  static struct termios new_settings;
  CHECKCL(tcgetattr(STDIN_FILENO, &old_settings) != -1, "Error at 'tcgetattr()'")
  new_settings = old_settings;
  new_settings.c_lflag &= ~((u32)ECHO);
  CHECKCL(tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) != -1, "Error at 'tcsetattr()'")
  i32 bytes_read;
  CHECKCL((bytes_read = read(STDIN_FILENO, password, MAX_PASSWORD_LEN)) != -1, "Error at reading the password from stdin")
  password[bytes_read] = '\0';
  for (i32 i = bytes_read; i >= 0; --i) {
	if (password[i] == '\n') {
	  password[i] = '\0';
	  break;
	}
  }
  CHECKCL(tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) != -1, "Error at 'tcsetattr()'")
  printf("\n");
}

void get_username_from_stdin(char *username) {
  fflush(stdout);
  printf("Username: ");
  fflush(stdout);
  i32 bytes_read;
  CHECKCL((bytes_read = read(STDIN_FILENO, username, MAX_USER_NAME_LEN)) != -1, "Error at reading the username from stdin")
  username[bytes_read] = '\0';
  for (i32 i = bytes_read; i >= 0; --i) {
	if (username[i] == '\n') {
	  username[i] = '\0';
	  break;
	}
  }
}

void get_credentials(char *username, char *password) {
  get_username_from_stdin(username);
  fflush(stdout);
  get_password_from_stdin(false, password);
  fflush(stdout);
}

bool is_repo_public_client(i32 server_socket_fd, const char *repository_name) {
  char is_repo_public_json[MAX_CHECK_VISIBILITY_LEN];
  CHECKCL(sprintf(is_repo_public_json, "{\"message_type\":\"is_repo_public_request\", \"repository_name\":\"%s\"}", repository_name) > 0,
		  "Error at 'sprintf()'")
  CHECKCL(write_with_prefix(server_socket_fd, is_repo_public_json, strlen(is_repo_public_json)) == true, "Cannot send the request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, is_repo_public_json) == true, "Cannot read the response from the server")
  JSON_Value *json_value = json_parse_string(is_repo_public_json);
  JSON_Object *json_object = json_value_get_object(json_value);
  bool is_public = json_object_dotget_boolean(json_object, "is_public");
  return is_public;
}
bool validate_address(const char *ip_address) {
  if (strlen(ip_address) > 15 || strlen(ip_address) < 7)
	return false;
  char section[4];
  if (ip_address == NULL) {
	return false;
  }
  i32 index = 0;
  u8 section_index = 0;
  u16 number;
  u8 dots = 0;
  char *pointer_end;
  bool accepting_digits = true;
  while (index < strlen(ip_address)) {
	if (accepting_digits == true) {
	  if (!(ip_address[index] >= '0' && ip_address[index] <= '9')) { return false; }
	  section[section_index] = ip_address[index];
	  section_index += 1;
	  if (section_index == 3 || (index + 1 < strlen(ip_address) && ip_address[index + 1] == '.')) {
		section[section_index] = '\0';
		number = (u16)strtol(section, &pointer_end, 10);
		if (!(number >= 0 && number <= 255)) {
		  return false;
		}
		section_index = 0;
		accepting_digits = false;
	  }
	} else if (ip_address[index] != '.') {
	  return false;
	} else {
	  accepting_digits = true;
	  dots += 1;
	}
	index += 1;
  }
  return dots == 3;
}

void copy_content_fd(i32 source_fd, i32 destination_fd) {
  char buffer[BUFSIZ];
  i32 bytes_read;
  while ((bytes_read = read_with_retry(source_fd, buffer, BUFSIZ)) != -1) {
	if (bytes_read == 0) {
	  break;
	}
	CHECKCL(write_with_retry(destination_fd, buffer, bytes_read) != bytes_read, "Internal error at writing in staging area")
  }
}

void show_help(const char *executable, bool exit_after_show) {
  printf("Usage : %s OPTION [ARGS,...]\nThese are common CMA commands used in various situations:\n"
		 "-	help - shows this message\n"
		 "-	init <repository_name> - creates a repository with the provided name\n"
		 "-	reset - resets the working directory files to the ones from the cloned version\n"
		 "-	stage-file <filename> - adds file to the staging area\n"
		 "-	unstage-file <filename> - removes file from the staging area\n"
		 "-	delete-file <filename> - removes file from the staging area, working directory and untouched directory\n"
		 "-	restore-file <filename> - restores the file in the working directory\n"
		 "-	append-to-changelog - appends a message to the current repository changelog\n"
		 "-	list-dirty - lists the files from the working directory that are modified\n"
		 "-	list-untouched - lists the files from the version of the repository that was cloned\n"
		 "-	list-staged - lists the files from the staging area\n"
		 "-	list-remote [-v <version>] - lists the files from remote repository, if no version is provided the latest one "
		 "is considered\n"
		 "-	register <username> <password> - creates a new account on the server\n"
		 "-	clone -n <repository-name - clones the remote specified repository(latest version)\n"
		 "-	pull - gets latest remote version of the current repository\n"
		 "-	diff-file <filename> [-v version] - gets the differences of current working directory file and one from the "
		 "server at specific version\n"
		 "-	diff-version {<version>|latest}- gets all the differences between the specified version and the one before it\n"
		 "-	checkout <version> - gets the latest version of the repository\n"
		 "-	checkout-file <filename> [-v version] - gets the file from the repository from the specific version, if none is "
		 "provided the latest one is considered\n"
		 "-	get-changelog <version> - gets the changelog of the current repository at specific version\n"
		 "-	allow-access <username> - allows access to the repository for a specific user if the repository is private\n"
		 "-	block-access <username> - blocks access to the repository for a specific user if the repository is private\n"
		 "-	allow-edit <username> - allows edit rights to a specific user for the current repository\n"
		 "-	block-edit <username> - removes edit rights of a specific user for the current repository\n"
		 "-	go-public - makes the repository public(only the owner can do this)\n"
		 "-	go-private - makes the repository private(only the owner can do this)\n"
		 "-	push - upload files from staging area to the server\n", executable);
  if (exit_after_show == true) { exit(EXIT_SUCCESS); }
}

bool is_internal_name(const char *filename) {
  return (strcmp(filename, ".staged") == 0 || strcmp(filename, ".untouched") == 0 || strcmp(filename, ".repo_config") == 0
	  || strcmp(filename, ".marked_as_deleted") == 0 || strcmp(filename, ".changelog") == 0);
}

void show_unknown_command(const char *executable, const char *command) {
  fprintf(stderr, "Unknown command: '%s'\n", command);
  show_help(executable, true);
}

void list_or_delete_only_files(const char *directory, bool delete_mode) {
  DIR *dir = NULL;
  struct dirent *de;
  struct stat st;
  if (stat(directory, &st) != 0 && delete_mode == true) { return; }
  CHECKCL(stat(directory, &st) == 0, "Error at stat")
  CHECKCL (NULL != (dir = opendir(directory)), "Error at opening directory")
  while (NULL != (de = readdir(dir))) {
	if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
	  if (S_ISREG(st.st_mode) != 0) {
		if (is_internal_name(de->d_name) == false) {
		  if (delete_mode == true) {
			char *ptr = (char *)(malloc(strlen(directory) + strlen(de->d_name) + 1));
			strcpy(ptr, directory);
			strcat(ptr, "/");
			strcat(ptr, de->d_name);
			CHECKCL(remove(ptr) == 0, "Cannot remove %s", ptr)
			free(ptr);
		  }
		  printf("- %s\n", de->d_name);
		}
	  }
	}
  }
  CHECKCL(closedir(dir) == 0, "Error at closing directory")
}

void push_files(const char *directory, JSON_Object *request_object) {
  DIR *dir = NULL;
  struct dirent *de;
  struct stat st;
  CHECKCL(stat(directory, &st) == 0, "Error at stat")
  CHECKCL (NULL != (dir = opendir(directory)), "Error at opening directory")
  JSON_Value *filenames_value = json_value_init_array();
  JSON_Array *filenames_array = json_value_get_array(filenames_value);
  JSON_Value *file_contents_value = json_value_init_array();
  JSON_Array *file_contents_array = json_value_get_array(file_contents_value);
  char *file_content = (char *)(malloc(MB5));
  while (NULL != (de = readdir(dir))) {
	if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
	  char *full_filepath = (char *)(malloc(strlen(directory) + strlen(de->d_name) + 1));
	  strcpy(full_filepath, directory);
	  strcat(full_filepath, "/");
	  strcat(full_filepath, de->d_name);
	  CHECKCL(stat(full_filepath, &st) == 0, "Error at stat for file %s", full_filepath)
	  if (S_ISREG(st.st_mode) != 0) {
		json_array_append_string(filenames_array, de->d_name);
		i32 file_descriptor = open(full_filepath, O_RDONLY);
		CHECKCL(file_descriptor != -1, "Could not open %s file", full_filepath)
		CHECKCL(read_with_retry(file_descriptor, file_content, st.st_size) == st.st_size, "Partial write could not be solved")
		CHECKCL(close(file_descriptor) == 0, "Could not close %s file", full_filepath)
		json_array_append_string(file_contents_array, file_content);
	  }
	  free(full_filepath);
	}
  }
  free(file_content);
  CHECKCL(closedir(dir) == 0, "Error at closing directory")
  json_object_set_value(request_object, "filenames", filenames_value);
  json_object_set_value(request_object, "filecontents", file_contents_value);
}

bool is_natural_number(const char *str) {
  if (strlen(str) > 1 && str[0] == '0') { return false; }
  for (u8 index = 0; str[index]; ++index) {
	if (!(str[index] >= '0' && str[index] <= '9')) {
	  return false;
	}
  }
  return true;
}

bool parse_non_connection_command_line(i32 argc, char **argv) {
  char *option = argv[1];
  char *conversion_ptr;
  char curr_working_dir[PATH_MAX];
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  char path_helper[PATH_MAX];
  struct stat st;
  if (strcmp(option, "help") == 0) {
	show_help(argv[0], false);
	return true;
  }
  if (strcmp(option, "serv-conf") == 0) {
	char serv_conf_file[MAX_FILE_PATH_LEN];
	struct passwd *pw = getpwuid(getuid());
	strcpy(serv_conf_file, pw->pw_dir);
	strcat(serv_conf_file, "/.cma_server");
	CHECKCL(argc == 4, "Usage: %s serv-conf ip_address port", argv[0])
	char *ip_address = argv[2];
	CHECKCL(validate_address(ip_address) == true, "You need to insert a valid ip address")
	u16 port = (u16)strtol(argv[3], &conversion_ptr, 10);
	JSON_Value *conf_value = json_value_init_object();
	JSON_Object *conf_object = json_value_get_object(conf_value);
	json_object_set_string(conf_object, "ip_address", ip_address);
	json_object_set_number(conf_object, "port", port);
	json_serialize_to_file_pretty(conf_value, serv_conf_file);
	return true;
  }
  if (strcmp(option, "init") == 0) {
	CHECKCL(argc == 2, "'%s' is a no arguments option, try again", option)
	CHECKCL(stat(".repo_config", &st) == -1, "There is already a repository created here, you cannot create other one")
	JSON_Value *conf_value = json_value_init_object();
	JSON_Object *conf_object = json_value_get_object(conf_value);
	json_object_set_string(conf_object, "repository_name", strrchr(curr_working_dir, '/') + 1);
	json_serialize_to_file_pretty(conf_value, ".repo_config");
	CHECKCL(mkdir(".staged", 0700) == 0, "Cannot create .staged directory")
	CHECKCL(mkdir(".untouched", 0700) == 0, "Cannot create .untouched directory")
	return true;
  }
  CHECKCL(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")
  if (strcmp(option, "reset") == 0) {
	CHECKCL(argc == 2, "'%s' is a no arguments option, try again", option)
	list_or_delete_only_files(curr_working_dir, true);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.staged");
	list_or_delete_only_files(path_helper, true);
	return true;
  }
  if (strcmp(option, "stage-file") == 0) {
	CHECKCL(argc == 3, "Usage: '%s' stage-file <filename>", option)
	CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCL(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCL(stat(argv[2], &st) != -1, "The file '%s' does not exist, cannot add", argv[2])
	CHECKCL(S_ISDIR(st.st_mode) != 0, "You cannot add directories")
	CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	i32 source_fd = open(argv[2], O_RDONLY);
	CHECKCL(source_fd != -1, "Cannot open file for reading '%s'", argv[2])
	strcpy(path_helper, ".staged/");
	strcat(path_helper, argv[2]);
	i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	copy_content_fd(source_fd, destination_fd);
	CHECKCL(close(source_fd) == 0, "Error at closing source fd for file %s", argv[2])
	CHECKCL(close(destination_fd) == 0, "Error at closing destination fd for file %s", argv[2])
	return true;
  }
  if (strcmp(option, "unstage-file") == 0) {
	CHECKCL(argc == 3, "Usage: '%s' unstage-file <filename>", option)
	CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCL(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	strcpy(path_helper, ".staged/");
	strcat(path_helper, argv[2]);
	CHECKCL(stat(path_helper, &st) != -1, "The file '%s' does not exist in the staging area, cannot unstage", argv[2])
	CHECKCL(remove(path_helper) != -1, "The file '%s' could not be removed from the staging area", path_helper)
	return true;
  }
  if (strcmp(option, "delete-file") == 0) {
	CHECKCL(argc == 3, "Usage: '%s' delete-file <filename>", option)
	CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCL(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	CHECKCL(stat(argv[2], &st) != -1, "The file '%s' does not exist, cannot delete", argv[2])
	JSON_Value *marked_as_deleted_value;
	JSON_Array *marked_as_deleted_array;
	if (stat(".marked_as_deleted", &st) != -1) {
	  marked_as_deleted_value = json_parse_file(".marked_as_deleted");
	  marked_as_deleted_array = json_value_get_array(marked_as_deleted_value);
	} else {
	  marked_as_deleted_value = json_value_init_array();
	  marked_as_deleted_array = json_value_get_array(marked_as_deleted_value);
	}
	json_array_append_string(marked_as_deleted_array, argv[2]);
	json_serialize_to_file_pretty(marked_as_deleted_value, ".marked_as_deleted");
	CHECKCL(remove(argv[2]) != -1, "The file '%s' could not be removed from the current working directory", argv[2])
	strcpy(path_helper, ".staged/");
	strcat(path_helper, argv[2]);
	if (stat(path_helper, &st) != -1) {
	  CHECKCL(remove(path_helper) != -1, "The file '%s' could not be removed from the staging area", path_helper)
	}
	return true;
  }
  if (strcmp(option, "restore-file") == 0) {
	CHECKCL(argc == 3, "Usage: '%s' restore-file <filename>", option)
	CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCL(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	strcpy(path_helper, ".untouched/");
	strcat(path_helper, argv[2]);
	CHECKCL(stat(path_helper, &st) != -1, "The file '%s' does not exist in the .untouched directory, cannot restore", argv[2])
	i32 source_fd = open(path_helper, O_RDONLY);
	i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(source_fd != -1, "Cannot open file for reading '%s'", path_helper)
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	copy_content_fd(source_fd, destination_fd);
	CHECKCL(close(source_fd) == 0, "Error at closing source fd for file %s", argv[2])
	CHECKCL(close(destination_fd) == 0, "Error at closing destination fd for file %s", argv[2])
	JSON_Value *marked_as_deleted_value = json_parse_file(".marked_as_deleted");
	JSON_Array *marked_as_deleted_array = json_value_get_array(marked_as_deleted_value);
	for (u32 index = 0; index < json_array_get_count(marked_as_deleted_array); ++index) {
	  if (strcmp(json_array_get_string(marked_as_deleted_array, index), argv[2]) == 0) {
		json_array_remove(marked_as_deleted_array, index);
		break;
	  }
	}
	json_serialize_to_file_pretty(marked_as_deleted_value, ".marked_as_deleted");
	return true;
  }
  if (strcmp(option, "append-to-changelog") == 0) {
	CHECKCL(argc == 3, "Usage: '%s' append-to-changelog <message>", option)
	JSON_Value *changelog_value;
	JSON_Array *changelog_array;
	if (stat(".changelog", &st) != -1) {
	  changelog_value = json_parse_file(".changelog");
	  changelog_array = json_value_get_array(changelog_value);
	} else {
	  changelog_value = json_value_init_array();
	  changelog_array = json_value_get_array(changelog_value);
	}
	json_array_append_string(changelog_array, argv[2]);
	json_serialize_to_file_pretty(changelog_value, ".changelog");
	return true;
  }
  if (strcmp(option, "list-dirty") == 0) {
	CHECKCL(argc == 2, "'%s' is a no arguments option, try again", option)
	list_or_delete_only_files(curr_working_dir, false);
	return true;
  }
  if (strcmp(option, "list-untouched") == 0) {
	CHECKCL(argc == 2, "'%s' is a no arguments option, try again", option)
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.untouched");
	list_or_delete_only_files(path_helper, false);
	return true;
  }
  if (strcmp(option, "list-staged") == 0) {
	CHECKCL(argc == 2, "'%s' is a no arguments option, try again", option)
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.staged");
	list_or_delete_only_files(path_helper, false);
	return true;
  }
  return false;
}
void write_files_to_disk(JSON_Object *json_response_object, char *path_helper, const char *curr_working_dir) {
  struct stat st;
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.staged");
  if (stat(path_helper, &st) == -1) {
	CHECKCL(mkdir(path_helper, 0700) == 0, "Cannot create .staged directory")
  }
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.untouched");
  if (stat(path_helper, &st) == -1) {
	CHECKCL(mkdir(path_helper, 0700) == 0, "Cannot create .untouched directory")
  }
  JSON_Array *file_contents_array = json_object_get_array(json_response_object, "filecontents");
  JSON_Array *file_names_array = json_object_get_array(json_response_object, "filenames");
  u32 no_of_files = json_array_get_count(file_names_array);
  i32 destination_fd;
  u32 file_size;
  const char *filename;
  const char *file_content;
  for (u32 index = 0; index < no_of_files; ++index) {
	filename = json_array_get_string(file_names_array, index);
	file_content = json_array_get_string(file_contents_array, index);
	file_size = strlen(file_content);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/");
	strcat(path_helper, filename);
	destination_fd = open(path_helper, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", path_helper)
	CHECKCL(write_with_retry(destination_fd, file_content, file_size) == file_size, "Cannot write the file content")
	CHECKCL(close(destination_fd) == 0, "Cannot close file %s", path_helper)
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.untouched/");
	strcat(path_helper, filename);
	destination_fd = open(path_helper, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", path_helper)
	CHECKCL(write_with_retry(destination_fd, file_content, file_size) == file_size, "Cannot write the file content")
	CHECKCL(close(destination_fd) == 0, "Cannot close file %s", path_helper)
  }
}
bool parse_connection_command_line(i32 server_socket_fd, i32 argc, char **argv) {
  char *option = argv[1];
  JSON_Value *json_request_value = json_value_init_object();
  JSON_Object *json_request_object = json_value_get_object(json_request_value);
  JSON_Value *json_response_value = NULL;
  JSON_Object *json_response_object = NULL;
  struct stat st;
  char curr_working_dir[PATH_MAX];
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  char path_helper[PATH_MAX];
  bool is_error;
  char *conversion_ptr;
  u16 version;
  char username[MAX_USER_NAME_LEN];
  char password[MAX_PASSWORD_LEN];
  if (strcmp(option, "register") == 0) {
	char register_json[SMALL_BUFFER];
	CHECKCL(argc == 2, "register is a no arguments option, try again")
	char password_confirm[MAX_PASSWORD_LEN];
	json_object_set_string(json_request_object, "message_type", "register_request");
	get_credentials(username, password);
	get_password_from_stdin(true, password_confirm);
	CHECKCL(strcmp(password, password_confirm) == 0, "The passwords do not match, try again")
	json_object_set_string(json_request_object, "username", username);
	json_object_set_string(json_request_object, "password", password);
	CHECKEXIT(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			  "Cannot send the user login details to the server")
	CHECKEXIT(read_with_prefix(server_socket_fd, register_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(register_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return true;
  }
  JSON_Value *conf_value = json_parse_file(".repo_config");
  JSON_Object *conf_object = json_value_get_object(conf_value);
  const char *repository_name = json_object_dotget_string(conf_object, "repository_name");
  json_object_dotset_string(json_request_object, "repository_name", repository_name);
  bool got_already_the_credentials = false;
  if (is_repo_public_client(server_socket_fd, repository_name) == false) {
	get_credentials(username, password);
	json_object_set_string(json_request_object, "username", username);
	json_object_set_string(json_request_object, "password", password);
	got_already_the_credentials = true;
  }
  if (strcmp(option, "list-remote") == 0) {
	char list_remote_json[MEDIUM_BUFFER];
	CHECKCL(argc == 2 || (argc == 4 && strcmp(argv[2], "-v") == 0 && is_natural_number(argv[3]) == true),
			"You must use list-remote with a specific version or none (latest will be considered)")
	json_object_set_string(json_request_object, "message_type", "list_remote_files_request");
	if (argc == 2) {
	  json_object_set_boolean(json_request_object, "has_version", false);
	} else {
	  version = (u16)strtol(argv[3], &conversion_ptr, 10);
	  json_object_set_boolean(json_request_object, "has_version", true);
	  json_object_set_number(json_request_object, "version", version);
	}
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the list-remote request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, list_remote_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(list_remote_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	JSON_Array *filenames_array = json_object_get_array(json_response_object, "files");
	for (u32 index = 0; index < json_array_get_count(filenames_array); ++index) {
	  printf("- %s\n", json_array_get_string(filenames_array, index));
	}
	return true;
  }
  if (strcmp(option, "clone") == 0) {
	CHECKCL(argc == 3 || argc == 4, "You must specify the repository you want to clone and optionally, the directory name")
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/");
	if (argc == 3) {
	  strcat(path_helper, argv[2]);
	} else {
	  strcat(path_helper, argv[3]);
	}
	const char *repo_local_name = argv[2];
	json_object_dotset_string(json_request_object, "message_type", "clone_request");
	json_object_dotset_string(json_request_object, "repository_name", repo_local_name);
	json_object_dotset_boolean(json_request_object, "has_version", false);
	char *clone_json = (char *)(malloc(MB20));
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the clone request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, clone_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(clone_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	CHECKCL(mkdir(path_helper, 0700) == 0, "Cannot create repository directory")
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.repo_config");
	JSON_Value *local_conf_value = json_value_init_object();
	JSON_Object *local_conf_object = json_value_get_object(local_conf_value);
	json_object_set_string(local_conf_object, "repository_name", repo_local_name);
	json_serialize_to_file_pretty(local_conf_value, path_helper);
	write_files_to_disk(json_response_object, path_helper, curr_working_dir);
	printf("Clone operation successfully made\n");
	free(clone_json);
	return true;
  }
  if (strcmp(option, "pull") == 0) {
	CHECKCL(argc == 2, "Usage: %s pull", argv[0])
	list_or_delete_only_files(curr_working_dir, true);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.untouched");
	list_or_delete_only_files(path_helper, true);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.staged");
	list_or_delete_only_files(path_helper, true);
	json_object_dotset_string(json_request_object, "message_type", "pull_request");
	json_object_dotset_string(json_request_object, "repository_name", repository_name);
	json_object_dotset_boolean(json_request_object, "has_version", false);
	char *pull_json = (char *)(malloc(MB20));
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the pull request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, pull_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(pull_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	write_files_to_disk(json_response_object, path_helper, curr_working_dir);
	printf("Pull operation successfully made, version: v%d\n", (u16)json_object_dotget_number(json_response_object, "version"));
	free(pull_json);
	return true;
  }
  if (strcmp(option, "diff-file") == 0) {
	CHECKCL(argc == 3 || (argc == 5 && strcmp(argv[3], "-v") == 0 && is_natural_number(argv[4]) == true),
			"You must provide a filename and optionally a version(none means latest version)")
	json_object_set_string(json_request_object, "message_type", "diff_request");
	json_object_set_string(json_request_object, "filename", argv[2]);
	if (argc == 3) {
	  json_object_set_boolean(json_request_object, "has_version", false);
	} else {
	  json_object_set_boolean(json_request_object, "has_version", true);
	  version = (u16)strtol(argv[4], &conversion_ptr, 10);
	  json_object_set_number(json_request_object, "version", version);
	}
	char *file_content_json = (char *)(calloc(MB5, sizeof(char)));
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the checkout-file request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, file_content_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(file_content_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	char temp_path[PATH_MAX];
	strcpy(temp_path, "/tmp/");
	strcat(temp_path, argv[2]);
	i32 destination_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	u32 content_size = json_object_dotget_string_len(json_response_object, "content");
	CHECKCL(write_with_retry(destination_fd, json_object_dotget_string(json_response_object, "content"), content_size) == content_size,
			"Could not write entire file %s to disk", temp_path)
	free(file_content_json);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/");
	strcat(path_helper, argv[2]);
	CHECKCL(execlp("diff", "diff", "-u", temp_path, path_helper, NULL) != -1, false, "Error at execlp()")
  }
  if (strcmp(option, "diff-version") == 0) {
	CHECKCL(argc == 3 && (is_natural_number(argv[2]) == true || strcmp(argv[2], "latest") == 0), "Usage: diff-version {<version>|latest}")
	json_object_set_string(json_request_object, "message_type", "get_differences_request");
	if (strcmp(argv[2], "latest") == 0) {
	  json_object_set_boolean(json_request_object, "has_version", false);
	} else {
	  json_object_set_boolean(json_request_object, "has_version", true);
	  version = (u16)strtol(argv[2], &conversion_ptr, 10);
	  CHECKCL(version != 0, "There is no 0 version, version numbers start with 1")
	  json_object_set_number(json_request_object, "version", version);
	}
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the checkout-file request to the server")
	char *difference_json = (char *)(malloc(MB10));
	CHECKCL(read_with_prefix(server_socket_fd, difference_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(difference_json);
	json_response_object = json_value_get_object(json_response_value);
	JSON_Array *filenames_array = json_object_get_array(json_response_object, "filenames");
	JSON_Array *differences_array = json_object_get_array(json_response_object, "differences");
	u32 length = json_array_get_count(filenames_array);
	version = (u16)json_object_dotget_number(json_response_object, "version");
	printf("Differences for between v%d and v%d\n", version, version - 1);
	for (u32 index = 0; index < length; ++index) {
	  printf("-> filename: %s\n", json_array_get_string(filenames_array, index));
	  printf("-> diff: %s\n", json_array_get_string(differences_array, index));
	}
	printf("End of section");
	free(difference_json);
	return true;
  }
  if (strcmp(option, "checkout") == 0) {
	CHECKCL(argc == 3 && is_natural_number(argv[2]) == true, "Usage: checkout <version>")
	version = (u16)(strtol(argv[2], &conversion_ptr, 10));
	list_or_delete_only_files(curr_working_dir, true);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.untouched");
	list_or_delete_only_files(path_helper, true);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.staged");
	list_or_delete_only_files(path_helper, true);
	json_object_dotset_string(json_request_object, "message_type", "checkout_request");
	json_object_dotset_string(json_request_object, "repository_name", repository_name);
	json_object_dotset_boolean(json_request_object, "has_version", true);
	json_object_dotset_number(json_request_object, "version", version);
	char *checkout_json = (char *)(malloc(MB20));
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the checkout request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, checkout_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(checkout_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	write_files_to_disk(json_response_object, path_helper, curr_working_dir);
	printf("Checkout operation successfully made, version: v%d\n", (u16)json_object_dotget_number(json_response_object, "version"));
	free(checkout_json);
	return true;
  }
  if (strcmp(option, "checkout-file") == 0) {
	CHECKCL(argc == 3 || (argc == 5 && strcmp(argv[3], "-v") == 0 && is_natural_number(argv[4]) == true),
			"You must provide a filename and optionally a version(none means latest version)")
	json_object_set_string(json_request_object, "message_type", "checkout_file_request");
	json_object_set_string(json_request_object, "filename", argv[2]);
	if (argc == 3) {
	  json_object_set_boolean(json_request_object, "has_version", false);
	} else {
	  json_object_set_boolean(json_request_object, "has_version", true);
	  version = (u16)strtol(argv[4], &conversion_ptr, 10);
	  json_object_set_number(json_request_object, "version", version);
	}
	char *file_content_json = (char *)(calloc(MB5, sizeof(char)));
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the checkout-file request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, file_content_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(file_content_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	u32 content_size = json_object_dotget_string_len(json_response_object, "content");
	CHECKCL(write_with_retry(destination_fd, json_object_dotget_string(json_response_object, "content"), content_size) == content_size,
			"Could not write entire file %s to disk", argv[2])
	free(file_content_json);
	return true;
  }
  if (strcmp(option, "get-changelog") == 0) {
	char *changelogs_json = (char *)(calloc(MB20, sizeof(char)));
	CHECKCL(argc == 2 || (argc == 3 && strcmp(argv[2], "all") == 0) || (argc == 4 && strcmp(argv[2], "-v") == 0 &&
		is_natural_number(argv[3]) == true), "You must use get-changelog with a specific version, all or none (latest will be considered)")
	json_object_set_string(json_request_object, "message_type", "get_changelog_request");
	if (argc == 2) {
	  json_object_set_boolean(json_request_object, "has_version", false);
	  json_object_set_boolean(json_request_object, "all_versions", false);
	} else if (argc == 3) {
	  json_object_set_boolean(json_request_object, "all_versions", true);
	  json_object_set_boolean(json_request_object, "has_version", false);
	} else {
	  version = (u16)strtol(argv[3], &conversion_ptr, 10);
	  json_object_set_boolean(json_request_object, "all_versions", false);
	  json_object_set_boolean(json_request_object, "has_version", true);
	  json_object_set_number(json_request_object, "version", version);
	}
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value),
							  json_serialization_size(json_request_value)) == true,
			"Cannot send the get-changelog request to the server")
	CHECKCL(read_with_prefix(server_socket_fd, changelogs_json) == true,
			"Cannot receive response from server")
	json_response_value = json_parse_string(changelogs_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	JSON_Array *changelogs_array = json_object_get_array(json_response_object, "changelogs");
	JSON_Array *unix_dates_array = json_object_get_array(json_response_object, "dates");
	JSON_Array *versions_array = json_object_get_array(json_response_object, "versions");
	u32 changelog_count = json_array_get_count(changelogs_array);
	for (u32 index = 0; index < changelog_count; ++index) {
	  printf("CHANGELOG [v%d]: %d:\n", (u32)json_array_get_number(versions_array, index), (u32)json_array_get_number(unix_dates_array, index));
	  JSON_Array *current_version_messages_array = json_value_get_array(json_parse_string(json_array_get_string(changelogs_array, index)));
	  u32 current_changelog_count = json_array_get_count(current_version_messages_array);
	  for (u32 index_current = 0; index_current < current_changelog_count; ++index_current) {
		printf("MSG{%d}: %s\n", current_changelog_count - index_current, json_array_get_string(current_version_messages_array, index_current));
	  }
	}
	return true;
  }
  if (got_already_the_credentials == false) {
	get_credentials(username, password);
  }
  json_object_set_string(json_request_object, "username", username);
  json_object_set_string(json_request_object, "password", password);
// this will always require credentials no matter the access specifier of the repository
  if (strcmp(option, "allow-access") == 0 || strcmp(option, "block-access") == 0 || strcmp(option, "allow-edit") == 0
	  || strcmp(option, "block-edit") == 0) {
	CHECKCL(argc == 3, "Usage: %s %s other_username", argv[0], argv[1])
	if (strcmp(option, "allow-access") == 0) {
	  json_object_set_string(json_request_object, "message_type", "allow_access_request");
	} else if (strcmp(option, "block-access") == 0) {
	  json_object_set_string(json_request_object, "message_type", "block_access_request");
	} else if (strcmp(option, "allow-edit") == 0) {
	  json_object_set_string(json_request_object, "message_type", "allow_edit_request");
	} else {// block-edit
	  json_object_set_string(json_request_object, "message_type", "block_edit_request");
	}
	json_object_set_string(json_request_object, "other_username", argv[2]);
	json_object_set_string(json_request_object, "repository_name", repository_name);
	char perms_json[SMALL_BUFFER];
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the credentials to the server")
	CHECKCL(read_with_prefix(server_socket_fd, perms_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(perms_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return true;
  }
  if (strcmp(option, "go-public") == 0 || strcmp(option, "go-private") == 0) {
	if (strcmp(option, "go-public") == 0) {
	  json_object_set_string(json_request_object, "message_type", "make_repo_public_request");
	} else {
	  json_object_set_string(json_request_object, "message_type", "make_repo_private_request");
	}
	json_object_set_string(json_request_object, "repository_name", repository_name);
	char privacy_json[SMALL_BUFFER];
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the credentials to the server")
	CHECKCL(read_with_prefix(server_socket_fd, privacy_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(privacy_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return true;
  }
  if (strcmp(option, "push") == 0) {
	CHECKCL(argc == 2, "push is a no arguments option, try again")
	json_object_set_string(json_request_object, "message_type", "push_request");
	CHECKCL(stat(".changelog", &st) == 0, "You must add at least one commit message(correct ways)")
	JSON_Value *changelog_json = json_parse_file(".changelog");
	json_object_set_value(json_request_object, "changelog", changelog_json);
	strcpy(path_helper, curr_working_dir);
	strcat(path_helper, "/.staged");
	push_files(path_helper, json_request_object);
	CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(json_request_value), json_serialization_size(json_request_value)) == true,
			"Cannot send the credentials to the server")
	char push_json[SMALL_BUFFER];
	CHECKCL(read_with_prefix(server_socket_fd, push_json) == true, "Cannot receive response from server")
	json_response_value = json_parse_string(push_json);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCL(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	CHECKCL(remove(".changelog") == 0, "Cannot remove changelog file")
	return true;
  }
  return false;
}
void parse_command_line(i32 argc, char **argv) {
  bool local_bool = false;
  local_bool = parse_non_connection_command_line(argc, argv);
  if (local_bool == true) {
	return;
  }
  struct stat st;
  char serv_conf_file[MAX_FILE_PATH_LEN];
  struct passwd *pw = getpwuid(getuid());
  strcpy(serv_conf_file, pw->pw_dir);
  strcat(serv_conf_file, "/.cma_server");
  CHECKCL(stat(serv_conf_file, &st) != -1, "Before doing network related operations you must set the remote server details")
  i32 server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_address;
  JSON_Value *conf_value = json_parse_file(serv_conf_file);
  JSON_Object *conf_object = json_value_get_object(conf_value);
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(json_object_dotget_number(conf_object, "port"));
  CHECKCL(inet_pton(AF_INET, json_object_dotget_string(conf_object, "ip_address"), &server_address.sin_addr.s_addr) == 1,
		  "Cannot convert str to valid address")
  CHECKCL(connect(server_socket_fd, (const struct sockaddr *)&server_address, sizeof(server_address)) == 0, "Cannot connect to the server")
  local_bool = parse_connection_command_line(server_socket_fd, argc, argv);
  CHECKCL(shutdown(server_socket_fd, SHUT_RDWR) == 0, "Cannot shutdown client")
  if (local_bool == false) {
	show_unknown_command(argv[0], argv[1]);
  }
}
