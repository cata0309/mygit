//
// Created by Marincia Cătălin on 23.12.2020.
//

#include "Commands.h"
#include "../common/Utils.h"

#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>

void util_read_password(bool confirm_stage, char *password) {
  fflush(stdout);
  if (confirm_stage == true) {
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

void util_read_username(char *username) {
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

void util_read_credentials(char *username, char *password) {
  util_read_username(username);
  fflush(stdout);
  util_read_password(false, password);
  fflush(stdout);
}

bool util_is_repository_public(i32 server_socket_fd, const char *repository_name) {
  char buffer[MAX_CHECK_VISIBILITY_LEN];
  bzero(buffer, MAX_CHECK_VISIBILITY_LEN);
  CHECKCL(sprintf(buffer, "{\"message_type\":\"is_repo_public_request\", \"repository_name\":\"%s\"}", repository_name) > 0,
		  "Error at 'sprintf()'")
  CHECKCL(write_with_prefix(server_socket_fd, buffer, strlen(buffer)) == true, "Cannot send the request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot read the response from the server")
  JSON_Value *json_value = json_parse_string(buffer);
  JSON_Object *json_object = json_value_get_object(json_value);
  bool is_public = json_object_dotget_boolean(json_object, "is_public");
  return is_public;
}

void util_copy_content_of_fd_to_fd(i32 source_fd, i32 destination_fd) {
  char buffer[BUFSIZ];
  bzero(buffer, BUFSIZ);
  i32 bytes_read;
  while ((bytes_read = read(source_fd, buffer, BUFSIZ)) != -1) {
	if (bytes_read == 0) {
	  break;
	}
	CHECKCL(write(destination_fd, buffer, bytes_read) == bytes_read, "Internal error at writing in staging area")
	bzero(buffer, BUFSIZ);
  }
}

void cmd_help(const char *executable, bool exit_after) {
  printf("Usage : %s OPTION [ARGS,...]\nThese are common CMA commands used in various situations:\n"
		 "- help - shows this message\n"
		 "- init - creates a repository with the current directory name\n"
		 "- reset - resets the working directory files to the ones from the cloned version\n"
		 "- stage-file <filename> - adds file to the staging area\n"
		 "- unstage-file <filename> - removes file from the staging area\n"
		 "- delete-file <filename> - removes file from the staging area, working directory and untouched directory\n"
		 "- restore-file <filename> - restores the file in the working directory\n"
		 "- append-message - appends a message to the current repository changelog\n"
		 "- list-dirty - lists the files from the working directory that are modified\n"
		 "- list-untouched - lists the files from the version of the repository that was cloned\n"
		 "- list-staged - lists the files from the staging area\n"
		 "- list-remote [-v <version>] - lists the files from remote repository, if no version is provided the latest one "
		 "is considered\n"
		 "- register <username> <password> - creates a new account on the server\n"
		 "- clone -n <repository-name - clones the remote specified repository(latest version)\n"
		 "- pull - gets latest remote version of the current repository\n"
		 "- diff-file <filename> [-v version] - gets the differences of current working directory file and one from the "
		 "server at specific version\n"
		 "- diff-version {<version>|latest}- gets all the differences between the specified version and the one before it\n"
		 "- checkout <version> - gets the latest version of the repository\n"
		 "- checkout-file <filename> [-v version] - gets the file from the repository from the specific version, if none is "
		 "provided the latest one is considered\n"
		 "- get-changelog {all| -v <version>} - gets the changelog of the current repository at specific version, no option means latest\n"
		 "- allow-access <username> - allows access to the repository for a specific user if the repository is private\n"
		 "- block-access <username> - blocks access to the repository for a specific user if the repository is private\n"
		 "- allow-edit <username> - allows edit rights to a specific user for the current repository\n"
		 "- block-edit <username> - removes edit rights of a specific user for the current repository\n"
		 "- go-public - makes the repository public(only the owner can do this)\n"
		 "- go-private - makes the repository private(only the owner can do this)\n"
		 "- push - upload files from staging area to the server\n", executable);
  if (exit_after == true) { exit(EXIT_SUCCESS); }
}

bool util_is_internal_name(const char *filename) {
  return (strcmp(filename, ".staged") == 0 || strcmp(filename, ".untouched") == 0 || strcmp(filename, ".repo_config") == 0
	  || strcmp(filename, ".marked_as_deleted") == 0 || strcmp(filename, ".changelog") == 0);
}

void cmd_unknown(const char *executable, const char *command) {
  fprintf(stderr, "Unknown command: '%s'\n", command);
  cmd_help(executable, true);
}

void util_list_or_delete_files(const char *directory, bool delete_mode) {
  DIR *dir = NULL;
  struct dirent *de;
  struct stat st;
  if (stat(directory, &st) != 0 && delete_mode == true) { return; }
  CHECKCL(stat(directory, &st) == 0, "Error at stat")
  CHECKCL (NULL != (dir = opendir(directory)), "Error at opening directory")
  while (NULL != (de = readdir(dir))) {
	if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
	  char *full_filepath = (char *)(malloc(strlen(directory) + strlen(de->d_name) + 1));
	  strcpy(full_filepath, directory);
	  strcat(full_filepath, "/");
	  strcat(full_filepath, de->d_name);
	  CHECKCL(stat(full_filepath, &st) == 0, "Error at stat for file %s", full_filepath)
	  if (S_ISREG(st.st_mode) != 0) {
		if (delete_mode == true) {
		  if (strcmp(de->d_name, ".repo_config") != 0) {
			CHECKCL(remove(full_filepath) == 0, "Cannot remove %s", full_filepath)
		  }
		} else {
		  if (util_is_internal_name(de->d_name) == false) {
			printf("- %s\n", de->d_name);
		  }
		}
	  }

	  free(full_filepath);
	}
  }
  CHECKCL(closedir(dir) == 0, "Error at closing directory")
}

i64 util_is_in_array(JSON_Array *array, char *str) {
  i32 length = json_array_get_count(array);
  for (i32 index = 0; index < length; ++index) {
	if (strcmp(str, json_array_get_string(array, index)) == 0) {
	  return index;
	}
  }
  return -1;
}

void util_push_populate(const char *working_directory, JSON_Object *request_object) {
  JSON_Value *filenames_value = json_value_init_array();
  JSON_Array *filenames_array = json_value_get_array(filenames_value);
  JSON_Value *file_contents_value = json_value_init_array();
  JSON_Array *file_contents_array = json_value_get_array(file_contents_value);
  JSON_Value *marked_as_deleted_value = NULL;
  JSON_Array *marked_as_deleted_array = NULL;
  struct stat st;
  bool check_if_deleted = false;
  if (stat(".marked_as_deleted", &st) == 0) {
	marked_as_deleted_value = json_parse_file(".marked_as_deleted");
	marked_as_deleted_array = json_value_get_array(marked_as_deleted_value);
	check_if_deleted = true;
  }
  char *file_content = (char *)(malloc(MB5));
  DIR *dir1, *dir2 = NULL;
  struct dirent *de1, *de2;
  char dot_untouched[PATH_MAX];
  bzero(dot_untouched, PATH_MAX);
  char dot_staged[PATH_MAX];
  bzero(dot_staged, PATH_MAX);
  strcpy(dot_untouched, working_directory);
  strcat(dot_untouched, "/.untouched");
  strcpy(dot_staged, working_directory);
  strcat(dot_staged, "/.staged");
  CHECKCL(stat(dot_untouched, &st) == 0, "Error at stat")
  CHECKCL(stat(dot_staged, &st) == 0, "Error at stat")
  CHECKCL (NULL != (dir1 = opendir(dot_staged)), "Error at opening directory")
  while (NULL != (de1 = readdir(dir1))) {
	if (strcmp(de1->d_name, ".") != 0 && strcmp(de1->d_name, "..") != 0) {
	  char *full_filepath = (char *)(malloc(strlen(dot_staged) + strlen(de1->d_name) + 1));
	  bzero(full_filepath, strlen(dot_staged) + strlen(de1->d_name) + 1);
	  strcpy(full_filepath, dot_staged);
	  strcat(full_filepath, "/");
	  strcat(full_filepath, de1->d_name);
	  CHECKCL(stat(full_filepath, &st) == 0, "Error at stat for file %s", full_filepath)
	  if (S_ISREG(st.st_mode) != 0) {
		json_array_append_string(filenames_array, de1->d_name);
		i32 source_fd = open(full_filepath, O_RDONLY);
		CHECKCL(source_fd != -1, "Could not open %s file", full_filepath)
		bzero(file_content, MB5);
		CHECKCL(read_with_retry(source_fd, file_content, st.st_size) == st.st_size, "Partial write could not be solved")
		char *untouched_filepath = (char *)(malloc(strlen(dot_untouched) + strlen(de1->d_name) + 1));
		bzero(full_filepath, strlen(dot_untouched) + strlen(de1->d_name) + 1);
		strcpy(untouched_filepath, dot_untouched);
		strcat(untouched_filepath, "/");
		strcat(untouched_filepath, de1->d_name);
		i32 destination_fd = open(untouched_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		CHECKCL(lseek(source_fd, 0, SEEK_SET) == 0, "Cannot move cursor to the beginning of the file")
		util_copy_content_of_fd_to_fd(source_fd, destination_fd);
		free(untouched_filepath);
		CHECKCL(close(source_fd) == 0, "Could not close %s file", full_filepath)
		CHECKCL(close(destination_fd) == 0, "Could not close %s file", full_filepath)
		json_array_append_string(file_contents_array, file_content);
	  }
	  free(full_filepath);
	}
  }
  CHECKCL(closedir(dir1) == 0, "Error at closing directory")
  CHECKCL (NULL != (dir2 = opendir(dot_untouched)), "Error at opening directory")
  while (NULL != (de2 = readdir(dir2))) {
	if (strcmp(de2->d_name, ".") != 0 && strcmp(de2->d_name, "..") != 0) {
	  if (check_if_deleted == false || util_is_in_array(marked_as_deleted_array, de2->d_name) == -1) {
		if (util_is_in_array(filenames_array, de2->d_name) == -1) {
		  char *full_filepath = (char *)(malloc(strlen(dot_untouched) + strlen(de2->d_name) + 1));
		  bzero(full_filepath, strlen(dot_untouched) + strlen(de2->d_name) + 1);
		  strcpy(full_filepath, dot_untouched);
		  strcat(full_filepath, "/");
		  strcat(full_filepath, de2->d_name);
		  CHECKCL(stat(full_filepath, &st) == 0, "Error at stat for file %s", full_filepath)
		  if (S_ISREG(st.st_mode) != 0) {
			json_array_append_string(filenames_array, de2->d_name);
			i32 file_descriptor = open(full_filepath, O_RDONLY);
			CHECKCL(file_descriptor != -1, "Could not open %s file", full_filepath)
			CHECKCL(read_with_retry(file_descriptor, file_content, st.st_size) == st.st_size, "Partial write could not be solved")
			CHECKCL(close(file_descriptor) == 0, "Could not close %s file", full_filepath)
			json_array_append_string(file_contents_array, file_content);
		  }
		  free(full_filepath);
		}
	  }
	}
  }
  CHECKCL(closedir(dir2) == 0, "Error at closing directory")
  json_object_set_value(request_object, "filenames", filenames_value);
  json_object_set_value(request_object, "filecontents", file_contents_value);
  free(file_content);
  util_list_or_delete_files(dot_staged, true);
}

bool util_is_natural_number(const char *str) {
  if (strlen(str) > 1 && str[0] == '0') { return false; }
  for (u8 index = 0; str[index]; ++index) {
	if (!(str[index] >= '0' && str[index] <= '9')) {
	  return false;
	}
  }
  return true;
}

void cmd_serv_conf(i32 argc, char **argv) {
  char *conversion_ptr;
  char serv_conf_file[MAX_FILE_PATH_LEN];
  bzero(serv_conf_file, MAX_FILE_PATH_LEN);
  struct passwd *pw = getpwuid(getuid());
  strcpy(serv_conf_file, pw->pw_dir);
  strcat(serv_conf_file, "/.aga_server");
  CHECKCL(argc == 4, "Usage: %s serv-conf ip_address port", argv[0])
  char *ip_address = argv[2];
  CHECKCL(util_is_address_valid(ip_address) == true, "You need to insert a valid ip address")
  u16 port = (u16)strtol(argv[3], &conversion_ptr, 10);
  JSON_Value *conf_value = json_value_init_object();
  JSON_Object *conf_object = json_value_get_object(conf_value);
  json_object_set_string(conf_object, "ip_address", ip_address);
  json_object_set_number(conf_object, "port", port);
  json_serialize_to_file_pretty(conf_value, serv_conf_file);
}

void cmd_init(i32 argc, char **argv) {
  CHECKCL(argc == 2, "'%s' is a no arguments option, try again", argv[1])
  struct stat st;
  CHECKCL(stat(".repo_config", &st) == -1, "There is already a repository created here, you cannot create other one")
  char curr_working_dir[PATH_MAX];
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  JSON_Value *conf_value = json_value_init_object();
  JSON_Object *conf_object = json_value_get_object(conf_value);
  json_object_set_string(conf_object, "repository_name", strrchr(curr_working_dir, '/') + 1);
  json_serialize_to_file_pretty(conf_value, ".repo_config");
  CHECKCL(mkdir(".staged", 0700) == 0, "Cannot create .staged directory")
  CHECKCL(mkdir(".untouched", 0700) == 0, "Cannot create .untouched directory")
}

void cmd_reset(i32 argc, char **argv) {
  CHECKCL(argc == 2, "'%s' is a no arguments option, try again", argv[1])
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  util_list_or_delete_files(curr_working_dir, true);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.staged");
  util_list_or_delete_files(path_helper, true);
  DIR *dir = NULL;
  struct dirent *de;
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.untouched");
  struct stat st;
  CHECKCL(stat(path_helper, &st) == 0, "Error at stat")
  CHECKCL (NULL != (dir = opendir(path_helper)), "Error at opening directory")
  while (NULL != (de = readdir(dir))) {
	if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
	  char *full_filepath = (char *)(malloc(strlen(path_helper) + strlen(de->d_name) + 1));
	  bzero(full_filepath, strlen(path_helper) + strlen(de->d_name) + 1);
	  strcpy(full_filepath, path_helper);
	  strcat(full_filepath, "/");
	  strcat(full_filepath, de->d_name);
	  CHECKCL(stat(full_filepath, &st) == 0, "Error at stat for file %s", full_filepath)
	  if (S_ISREG(st.st_mode) != 0) {
		i32 source_fd = open(full_filepath, O_RDONLY);
		i32 destination_fd = open(de->d_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		util_copy_content_of_fd_to_fd(source_fd, destination_fd);
		CHECKCL(close(source_fd) == 0, "Could not close %s file", full_filepath)
		CHECKCL(close(destination_fd) == 0, "Could not close %s file", full_filepath)
	  }
	  free(full_filepath);
	}
  }
  CHECKCL(closedir(dir) == 0, "Error at closing directory")
}

void cmd_stage_file(i32 argc, char **argv) {
  CHECKCL(argc == 3, "Usage: '%s' stage-file <filename>", argv[1])
  CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
  CHECKCL(!util_is_internal_name(argv[2]), "You cannot operate with tool files")
  struct stat st;
  CHECKCL(stat(argv[2], &st) != -1, "The file '%s' does not exist, cannot add", argv[2])
  CHECKCL(S_ISREG(st.st_mode) != 0, "You cannot add only regular files")
  CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
  i32 source_fd = open(argv[2], O_RDONLY);
  CHECKCL(source_fd != -1, "Cannot open file for reading '%s'", argv[2])
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  strcpy(curr_working_dir, ".staged/");
  strcat(curr_working_dir, argv[2]);
  i32 destination_fd = open(curr_working_dir, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
  util_copy_content_of_fd_to_fd(source_fd, destination_fd);
  CHECKCL(close(source_fd) == 0, "Error at closing source fd for file %s", argv[2])
  CHECKCL(close(destination_fd) == 0, "Error at closing destination fd for file %s", argv[2])
}

void cmd_unstage_file(i32 argc, char **argv) {
  CHECKCL(argc == 3, "Usage: '%s' unstage-file <filename>", argv[1])
  CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
  CHECKCL(!util_is_internal_name(argv[2]), "You cannot operate with tool files")
  CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, ".staged/");
  strcat(path_helper, argv[2]);
  struct stat st;
  CHECKCL(stat(path_helper, &st) != -1, "The file '%s' does not exist in the staging area, cannot unstage", argv[2])
  CHECKCL(remove(path_helper) != -1, "The file '%s' could not be removed from the staging area", path_helper)
}

void cmd_delete_file(i32 argc, char **argv) {
  CHECKCL(argc == 3, "Usage: '%s' delete-file <filename>", argv[1])
  CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
  CHECKCL(!util_is_internal_name(argv[2]), "You cannot operate with tool files")
  CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
  struct stat st;
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
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, ".staged/");
  strcat(path_helper, argv[2]);
  if (stat(path_helper, &st) != -1) {
	CHECKCL(remove(path_helper) != -1, "The file '%s' could not be removed from the staging area", path_helper)
  }
}

void cmd_restore_file(i32 argc, char **argv) {
  CHECKCL(argc == 3, "Usage: '%s' restore-file <filename>", argv[1])
  CHECKCL(!strchr(argv[2], '/'), "You can use only local files")
  CHECKCL(!util_is_internal_name(argv[2]), "You cannot operate with tool files")
  CHECKCL(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, ".untouched/");
  strcat(path_helper, argv[2]);
  struct stat st;
  CHECKCL(stat(path_helper, &st) != -1, "The file '%s' does not exist in the .untouched directory, cannot restore", argv[2])
  i32 source_fd = open(path_helper, O_RDONLY);
  i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  CHECKCL(source_fd != -1, "Cannot open file for reading '%s'", path_helper)
  CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
  util_copy_content_of_fd_to_fd(source_fd, destination_fd);
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
}

void cmd_append_message(i32 argc, char **argv) {
  CHECKCL(argc == 3, "Usage: '%s' append-message <message>", argv[1])
  JSON_Value *changelog_value;
  JSON_Array *changelog_array;
  struct stat st;
  if (stat(".changelog", &st) != -1) {
	changelog_value = json_parse_file(".changelog");
	changelog_array = json_value_get_array(changelog_value);
  } else {
	changelog_value = json_value_init_array();
	changelog_array = json_value_get_array(changelog_value);
  }
  json_array_append_string(changelog_array, argv[2]);
  json_serialize_to_file_pretty(changelog_value, ".changelog");
}

void cmd_list_dirty(i32 argc, char **argv) {
  CHECKCL(argc == 2, "'%s' is a no arguments option, try again", argv[1])
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  util_list_or_delete_files(curr_working_dir, false);
}

void cmd_list_untouched(i32 argc, char **argv) {
  CHECKCL(argc == 2, "'%s' is a no arguments option, try again", argv[1])
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  strcat(curr_working_dir, "/.untouched");
  util_list_or_delete_files(curr_working_dir, false);
}

void cmd_list_staged(i32 argc, char **argv) {
  CHECKCL(argc == 2, "'%s' is a no arguments option, try again", argv[1])
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  strcat(curr_working_dir, "/.staged");
  util_list_or_delete_files(curr_working_dir, false);
}

bool util_is_non_connection_cmd(const char *option) {
  return (strcmp(option, "help") == 0 || strcmp(option, "serv-conf") == 0 || strcmp(option, "init") == 0 || strcmp(option, "reset") == 0 ||
	  strcmp(option, "stage-file") == 0 || strcmp(option, "unstage-file") == 0 || strcmp(option, "delete-file") == 0 ||
	  strcmp(option, "restore-file") == 0 || strcmp(option, "append-message") == 0 || strcmp(option, "list-dirty") == 0 ||
	  strcmp(option, "list-untouched") == 0 || strcmp(option, "list-staged") == 0);
}

bool cmd_no_connection_distributor(i32 argc, char **argv) {
  char *option = argv[1];
  if (util_is_non_connection_cmd(option) == false) { return false; }
  if (strcmp(option, "help") == 0) {
	cmd_help(argv[0], false);
	return true;
  }
  if (strcmp(option, "serv-conf") == 0) {
	cmd_serv_conf(argc, argv);
	return true;
  }
  if (strcmp(option, "init") == 0) {
	cmd_init(argc, argv);
	return true;
  }
  struct stat st;
  CHECKCL(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")
  if (strcmp(option, "reset") == 0) {
	cmd_reset(argc, argv);
	return true;
  }
  if (strcmp(option, "stage-file") == 0) {
	cmd_stage_file(argc, argv);
	return true;
  }
  if (strcmp(option, "unstage-file") == 0) {
	cmd_unstage_file(argc, argv);
	return true;
  }
  if (strcmp(option, "delete-file") == 0) {
	cmd_delete_file(argc, argv);
	return true;
  }
  if (strcmp(option, "restore-file") == 0) {
	cmd_restore_file(argc, argv);
	return true;
  }
  if (strcmp(option, "append-message") == 0) {
	cmd_append_message(argc, argv);
	return true;
  }
  if (strcmp(option, "list-dirty") == 0) {
	cmd_list_dirty(argc, argv);
	return true;
  }
  if (strcmp(option, "list-untouched") == 0) {
	cmd_list_untouched(argc, argv);
	return true;
  }
  if (strcmp(option, "list-staged") == 0) {
	cmd_list_staged(argc, argv);
	return true;
  }
  return false;
}

void util_write_files_to_disk(JSON_Object *response_object, const char *repo_directory) {
  struct stat st;
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, repo_directory);
  strcat(path_helper, "/.staged");
  if (stat(path_helper, &st) == -1) {
	CHECKCL(mkdir(path_helper, 0700) == 0, "Cannot create .staged directory")
  }
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, repo_directory);
  strcat(path_helper, "/.untouched");
  if (stat(path_helper, &st) == -1) {
	CHECKCL(mkdir(path_helper, 0700) == 0, "Cannot create .untouched directory")
  }
  JSON_Array *file_contents_array = json_object_get_array(response_object, "filecontents");
  JSON_Array *file_names_array = json_object_get_array(response_object, "filenames");
  u32 no_of_files = json_array_get_count(file_names_array);
  i32 destination_fd;
  u32 file_size;
  const char *filename, *file_content;
  for (u32 index = 0; index < no_of_files; ++index) {
	filename = json_array_get_string(file_names_array, index);
	file_content = json_array_get_string(file_contents_array, index);
	file_size = strlen(file_content);
	bzero(path_helper, PATH_MAX);
	strcpy(path_helper, repo_directory);
	strcat(path_helper, "/");
	strcat(path_helper, filename);
	destination_fd = open(path_helper, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", path_helper)
	CHECKCL(write_with_retry(destination_fd, file_content, file_size) == file_size, "Cannot write the file content")
	CHECKCL(close(destination_fd) == 0, "Cannot close file %s", path_helper)
	bzero(path_helper, PATH_MAX);
	strcpy(path_helper, repo_directory);
	strcat(path_helper, "/.untouched/");
	strcat(path_helper, filename);
	destination_fd = open(path_helper, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", path_helper)
	CHECKCL(write_with_retry(destination_fd, file_content, file_size) == file_size, "Cannot write the file content")
	CHECKCL(close(destination_fd) == 0, "Cannot close file %s", path_helper)
  }
}

void cmd_list_remote(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 2 || (argc == 4 && strcmp(argv[2], "-v") == 0 && util_is_natural_number(argv[3]) == true),
		  "You must use list-remote with a specific version or none (latest will be considered)")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char buffer[MEDIUM_BUFFER];
  bzero(buffer, MEDIUM_BUFFER);
  char *conversion_ptr;
  bool is_error;
  u16 version;
  json_object_set_string(request_object, "message_type", "list_remote_files_request");
  if (argc == 2) {
	json_object_set_boolean(request_object, "has_version", false);
  } else {
	version = (u16)strtol(argv[3], &conversion_ptr, 10);
	json_object_set_boolean(request_object, "has_version", true);
	json_object_set_number(request_object, "version", version);
  }
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the list-remote request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
  JSON_Array *filenames_array = json_object_get_array(response_object, "files");
  for (u32 index = 0; index < json_array_get_count(filenames_array); ++index) {
	printf("- %s\n", json_array_get_string(filenames_array, index));
  }
}

void cmd_clone(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3 || argc == 4, "You must specify the repository you want to clone and optionally, the directory name")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  bool is_error;
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  CHECKCL(getcwd(path_helper, sizeof(path_helper)) != NULL, "Cannot get the current directory path")
  char repo_directory[PATH_MAX];
  strcpy(repo_directory, path_helper);
  strcat(repo_directory, "/");
  if (argc == 3) {
	strcat(repo_directory, argv[2]);
  } else {
	strcat(repo_directory, argv[3]);
  }
  const char *repo_local_name = argv[2];
  if (util_is_repository_public(server_socket_fd, repo_local_name) == false) {
	char username[MAX_USER_NAME_LEN];
	char password[MAX_PASSWORD_LEN];
	bzero(username, MAX_USER_NAME_LEN);
	bzero(password, MAX_PASSWORD_LEN);
	util_read_credentials(username, password);
	json_object_set_string(request_object, "username", username);
	json_object_set_string(request_object, "password", password);
  }
  json_object_dotset_string(request_object, "message_type", "clone_request");
  json_object_dotset_string(request_object, "repository_name", repo_local_name);
  json_object_dotset_boolean(request_object, "has_version", false);
  char *buffer = (char *)(malloc(MB20));
  bzero(buffer, MB20);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the clone request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  CHECKCL(mkdir(repo_directory, 0700) == 0, "Cannot create repository directory")
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, repo_directory);
  strcat(path_helper, "/.repo_config");
  JSON_Value *local_conf_value = json_value_init_object();
  JSON_Object *local_conf_object = json_value_get_object(local_conf_value);
  json_object_set_string(local_conf_object, "repository_name", repo_local_name);
  json_serialize_to_file_pretty(local_conf_value, path_helper);
  util_write_files_to_disk(response_object, repo_directory);
  printf("Clone operation successfully made\n");
  free(buffer);
}

void cmd_pull(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 2, "Usage: %s pull", argv[0])
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  bool is_error;
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  util_list_or_delete_files(curr_working_dir, true);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.untouched");
  util_list_or_delete_files(path_helper, true);
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.staged");
  util_list_or_delete_files(path_helper, true);
  json_object_dotset_string(request_object, "message_type", "pull_request");
  json_object_dotset_boolean(request_object, "has_version", false);
  char *buffer = (char *)(malloc(MB20));
  bzero(buffer, MB20);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the pull request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  util_write_files_to_disk(response_object, curr_working_dir);
  printf("Pull operation successfully made, version: v%d\n", (u16)json_object_dotget_number(response_object, "version"));
  free(buffer);
}

void cmd_diff_file(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3 || (argc == 5 && strcmp(argv[3], "-v") == 0 && util_is_natural_number(argv[4]) == true),
		  "You must provide a filename and optionally a version(none means latest version)")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  bool is_error;
  char *conversion_ptr;
  u16 version;
  json_object_set_string(request_object, "message_type", "diff_request");
  json_object_set_string(request_object, "filename", argv[2]);
  if (argc == 3) {
	json_object_set_boolean(request_object, "has_version", false);
  } else {
	json_object_set_boolean(request_object, "has_version", true);
	version = (u16)strtol(argv[4], &conversion_ptr, 10);
	json_object_set_number(request_object, "version", version);
  }
  char *buffer = (char *)(calloc(MB5, sizeof(char)));
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the checkout-file request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  char temp_path[PATH_MAX];
  bzero(temp_path, PATH_MAX);
  strcpy(temp_path, "/tmp/");
  strcat(temp_path, argv[2]);
  i32 destination_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
  u32 content_size = json_object_dotget_string_len(response_object, "content");
  CHECKCL(write_with_retry(destination_fd, json_object_dotget_string(response_object, "content"), content_size) == content_size,
		  "Could not write entire file %s to disk", temp_path)
  free(buffer);
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/");
  strcat(path_helper, argv[2]);
  CHECKCL(execlp("diff", "diff", "-u", temp_path, path_helper, NULL) != -1, false, "Error at execlp()")
}

void cmd_diff_version(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3 && (util_is_natural_number(argv[2]) == true || strcmp(argv[2], "latest") == 0), "Usage: diff-version {<version>|latest}")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *conversion_ptr;
  u16 version;
  json_object_set_string(request_object, "message_type", "get_differences_request");
  if (strcmp(argv[2], "latest") == 0) {
	json_object_set_boolean(request_object, "has_version", false);
  } else {
	json_object_set_boolean(request_object, "has_version", true);
	version = (u16)strtol(argv[2], &conversion_ptr, 10);
	CHECKCL(version != 0, "There is no 0 version, version numbers start with 1")
	json_object_set_number(request_object, "version", version);
  }
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the checkout-file request to the server")
  char *buffer = (char *)(malloc(MB10));
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  JSON_Array *filenames_array = json_object_get_array(response_object, "filenames");
  JSON_Array *differences_array = json_object_get_array(response_object, "differences");
  u32 length = json_array_get_count(filenames_array);
  version = (u16)json_object_dotget_number(response_object, "version");
  printf("Differences for between v%d and v%d\n", version, version - 1);
  for (u32 index = 0; index < length; ++index) {
	printf("-> filename: %s\n", json_array_get_string(filenames_array, index));
	printf("-> diff: %s\n", json_array_get_string(differences_array, index));
  }
  printf("End of section\n");
  free(buffer);
}

void cmd_checkout(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3 && util_is_natural_number(argv[2]) == true, "Usage: checkout <version>")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *conversion_ptr;
  u16 version;
  bool is_error;
  version = (u16)(strtol(argv[2], &conversion_ptr, 10));
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  char path_helper[PATH_MAX];
  bzero(path_helper, PATH_MAX);
  util_list_or_delete_files(curr_working_dir, true);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.untouched");
  util_list_or_delete_files(path_helper, true);
  bzero(path_helper, PATH_MAX);
  strcpy(path_helper, curr_working_dir);
  strcat(path_helper, "/.staged");
  util_list_or_delete_files(path_helper, true);
  json_object_dotset_string(request_object, "message_type", "checkout_request");
  json_object_dotset_boolean(request_object, "has_version", true);
  json_object_dotset_number(request_object, "version", version);
  char *buffer = (char *)(malloc(MB20));
  bzero(buffer, MB20);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the checkout request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  util_write_files_to_disk(response_object, curr_working_dir);
  printf("Checkout operation successfully made, version: v%d\n", (u16)json_object_dotget_number(response_object, "version"));
  free(buffer);
}

void cmd_checkout_file(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3 || (argc == 5 && strcmp(argv[3], "-v") == 0 && util_is_natural_number(argv[4]) == true),
		  "You must provide a filename and optionally a version(none means latest version)")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *conversion_ptr;
  u16 version;
  bool is_error;
  json_object_set_string(request_object, "message_type", "checkout_file_request");
  json_object_set_string(request_object, "filename", argv[2]);
  if (argc == 3) {
	json_object_set_boolean(request_object, "has_version", false);
  } else {
	json_object_set_boolean(request_object, "has_version", true);
	version = (u16)strtol(argv[4], &conversion_ptr, 10);
	json_object_set_number(request_object, "version", version);
  }
  char *buffer = (char *)(calloc(MB5, sizeof(char)));
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the checkout-file request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  CHECKCL(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
  u32 content_size = json_object_dotget_string_len(response_object, "content");
  CHECKCL(write_with_retry(destination_fd, json_object_dotget_string(response_object, "content"), content_size) == content_size,
		  "Could not write entire file %s to disk", argv[2])
  free(buffer);
}

void cmd_get_changelog(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 2 || (argc == 3 && strcmp(argv[2], "all") == 0) || (argc == 4 && strcmp(argv[2], "-v") == 0 &&
	  util_is_natural_number(argv[3]) == true), "You must use get-changelog with a specific version, all or none (latest will be considered)")
  char *buffer = (char *)(calloc(MB20, sizeof(char)));
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *conversion_ptr;
  u16 version;
  bool is_error;
  json_object_set_string(request_object, "message_type", "get_changelog_request");
  if (argc == 2) {
	json_object_set_boolean(request_object, "has_version", false);
	json_object_set_boolean(request_object, "all_versions", false);
  } else if (argc == 3) {
	json_object_set_boolean(request_object, "all_versions", true);
	json_object_set_boolean(request_object, "has_version", false);
  } else {
	version = (u16)strtol(argv[3], &conversion_ptr, 10);
	json_object_set_boolean(request_object, "all_versions", false);
	json_object_set_boolean(request_object, "has_version", true);
	json_object_set_number(request_object, "version", version);
  }
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the get-changelog request to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
  JSON_Array *changelogs_array = json_object_get_array(response_object, "changelogs");
  JSON_Array *unix_dates_array = json_object_get_array(response_object, "dates");
  JSON_Array *versions_array = json_object_get_array(response_object, "versions");
  u32 changelog_count = json_array_get_count(changelogs_array);
  for (u32 index = 0; index < changelog_count; ++index) {
	printf("CHANGELOG [v%d]: %d:\n", (u32)json_array_get_number(versions_array, index), (u32)json_array_get_number(unix_dates_array, index));
	JSON_Array *current_version_messages_array = json_value_get_array(json_parse_string(json_array_get_string(changelogs_array, index)));
	u32 current_changelog_count = json_array_get_count(current_version_messages_array);
	for (u32 index_current = 0; index_current < current_changelog_count; ++index_current) {
	  printf("MSG{%d}: %s\n", current_changelog_count - index_current, json_array_get_string(current_version_messages_array, index_current));
	}
  }
}

void cmd_switch_access_or_edit(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 3, "Usage: %s %s other_username", argv[0], argv[1])
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *option = argv[1];
  bool is_error;
  if (strcmp(option, "allow-access") == 0) {
	json_object_set_string(request_object, "message_type", "allow_access_request");
  } else if (strcmp(option, "block-access") == 0) {
	json_object_set_string(request_object, "message_type", "block_access_request");
  } else if (strcmp(option, "allow-edit") == 0) {
	json_object_set_string(request_object, "message_type", "allow_edit_request");
  } else {// block-edit
	json_object_set_string(request_object, "message_type", "block_edit_request");
  }
  json_object_set_string(request_object, "other_username", argv[2]);
  char buffer[SMALL_BUFFER];
  bzero(buffer, SMALL_BUFFER);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the credentials to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
}

void cmd_switch_repository_access(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value) {
  CHECKCL(argc == 2, "%s is a no argument option", argv[1])
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char *option = argv[1];
  bool is_error;
  if (strcmp(option, "go-public") == 0) {
	json_object_set_string(request_object, "message_type", "make_repo_public_request");
  } else {
	json_object_set_string(request_object, "message_type", "make_repo_private_request");
  }
  char buffer[SMALL_BUFFER];
  bzero(buffer, SMALL_BUFFER);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the credentials to the server")
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
}

void cmd_push(i32 server_socket_fd, i32 argc, JSON_Value *request_value) {
  CHECKCL(argc == 2, "push is a no arguments option, try again")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  bool is_error;
  json_object_set_string(request_object, "message_type", "push_request");
  struct stat st;
  CHECKCL(stat(".changelog", &st) == 0, "You must add at least one commit message(correct ways)")
  JSON_Value *changelog_json = json_parse_file(".changelog");
  json_object_set_value(request_object, "changelog", changelog_json);
  char curr_working_dir[PATH_MAX];
  bzero(curr_working_dir, PATH_MAX);
  CHECKCL(getcwd(curr_working_dir, sizeof(curr_working_dir)) != NULL, "Cannot get the current directory path")
  util_push_populate(curr_working_dir, request_object);
  CHECKCL(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
		  "Cannot send the credentials to the server")
  char buffer[SMALL_BUFFER];
  bzero(buffer, SMALL_BUFFER);
  CHECKCL(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
  if (stat(".marked_as_deleted", &st) == 0) {
	CHECKCL(remove(".marked_as_deleted") == 0, "Cannot delete marker for deleted files")
  }
  CHECKCL(remove(".changelog") == 0, "Cannot remove changelog file")
}

void cmd_register(i32 server_socket_fd, i32 argc, JSON_Value *request_value) {
  char buffer[SMALL_BUFFER];
  bzero(buffer, SMALL_BUFFER);
  CHECKCL(argc == 2, "register is a no arguments option, try again")
  JSON_Object *request_object = json_value_get_object(request_value);
  JSON_Value *response_value = NULL;
  JSON_Object *response_object = NULL;
  char username[MAX_USER_NAME_LEN];
  bzero(username, MAX_USER_NAME_LEN);
  char password[MAX_PASSWORD_LEN];
  bzero(password, MAX_PASSWORD_LEN);
  bool is_error;
  char password_confirm[MAX_PASSWORD_LEN];
  bzero(password_confirm, MAX_PASSWORD_LEN);
  json_object_set_string(request_object, "message_type", "register_request");
  util_read_credentials(username, password);
  util_read_password(true, password_confirm);
  CHECKCL(strcmp(password, password_confirm) == 0, "The passwords do not match, try again")
  json_object_set_string(request_object, "username", username);
  json_object_set_string(request_object, "password", password);
  CHECKEXIT(write_with_prefix(server_socket_fd, json_serialize_to_string(request_value), json_serialization_size(request_value)) == true,
			"Cannot send the user login details to the server")
  CHECKEXIT(read_with_prefix(server_socket_fd, buffer) == true, "Cannot receive response from server")
  response_value = json_parse_string(buffer);
  response_object = json_value_get_object(response_value);
  is_error = json_object_dotget_boolean(response_object, "is_error");
  CHECKCL(is_error == false, "%s", json_object_dotget_string(response_object, "message"))
  printf("%s\n", json_object_dotget_string(response_object, "message"));
}

bool util_is_connection_cmd(const char *option) {
  return (strcmp(option, "register") == 0 || strcmp(option, "list-remote") == 0 || strcmp(option, "clone") == 0 || strcmp(option, "pull") == 0 ||
	  strcmp(option, "diff-file") == 0 || strcmp(option, "diff-version") == 0 || strcmp(option, "checkout") == 0 ||
	  strcmp(option, "checkout-file") == 0 || strcmp(option, "get-changelog") == 0 || strcmp(option, "allow-access") == 0 ||
	  strcmp(option, "block-access") == 0 || strcmp(option, "allow-edit") == 0 || strcmp(option, "block-edit") == 0 ||
	  strcmp(option, "go-public") == 0 || strcmp(option, "go-private") == 0 || strcmp(option, "push") == 0);
}

bool cmd_connection_distributor(i32 server_socket_fd, i32 argc, char **argv) {
  char *option = argv[1];
  if (util_is_connection_cmd(option) == false) { return false; }
  JSON_Value *request_value = json_value_init_object();
  JSON_Object *request_object = json_value_get_object(request_value);
  char username[MAX_USER_NAME_LEN];
  bzero(username, MAX_USER_NAME_LEN);
  char password[MAX_PASSWORD_LEN];
  bzero(password, MAX_PASSWORD_LEN);
  if (strcmp(option, "register") == 0) {
	cmd_register(server_socket_fd, argc, request_value);
	return true;
  }
  if (strcmp(option, "clone") == 0) {
	cmd_clone(server_socket_fd, argc, argv, request_value);
	return true;
  }
  struct stat st;
  CHECKCL(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")
  JSON_Value *conf_value = json_parse_file(".repo_config");
  JSON_Object *conf_object = json_value_get_object(conf_value);
  const char *repository_name = json_object_dotget_string(conf_object, "repository_name");
  json_object_dotset_string(request_object, "repository_name", repository_name);
  bool got_already_the_credentials = false;
  if (util_is_repository_public(server_socket_fd, repository_name) == false) {
	util_read_credentials(username, password);
	json_object_set_string(request_object, "username", username);
	json_object_set_string(request_object, "password", password);
	got_already_the_credentials = true;
  }
  if (strcmp(option, "list-remote") == 0) {
	cmd_list_remote(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "pull") == 0) {
	cmd_pull(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "diff-file") == 0) {
	cmd_diff_file(server_socket_fd, argc, argv, request_value);
  }
  if (strcmp(option, "diff-version") == 0) {
	cmd_diff_version(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "checkout") == 0) {
	cmd_checkout(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "checkout-file") == 0) {
	cmd_checkout_file(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "get-changelog") == 0) {
	cmd_get_changelog(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (got_already_the_credentials == false) {
	util_read_credentials(username, password);
  }
  json_object_set_string(request_object, "username", username);
  json_object_set_string(request_object, "password", password);
// this will always require credentials no matter the access specifier of the repository
  if (strcmp(option, "allow-access") == 0 || strcmp(option, "block-access") == 0 || strcmp(option, "allow-edit") == 0
	  || strcmp(option, "block-edit") == 0) {
	cmd_switch_access_or_edit(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "go-public") == 0 || strcmp(option, "go-private") == 0) {
	cmd_switch_repository_access(server_socket_fd, argc, argv, request_value);
	return true;
  }
  if (strcmp(option, "push") == 0) {
	cmd_push(server_socket_fd, argc, request_value);
	return true;
  }
  return false;
}

void cmd_distributor(i32 argc, char **argv) {
  bool helper_bool = false;
  helper_bool = cmd_no_connection_distributor(argc, argv);
  if (helper_bool == true) {
	return;
  }
  struct stat st;
  char serv_conf_file[MAX_FILE_PATH_LEN];
  bzero(serv_conf_file, MAX_FILE_PATH_LEN);
  struct passwd *pw = getpwuid(getuid());
  strcpy(serv_conf_file, pw->pw_dir);
  strcat(serv_conf_file, "/.aga_server");
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
  helper_bool = cmd_connection_distributor(server_socket_fd, argc, argv);
  CHECKCL(shutdown(server_socket_fd, SHUT_RDWR) == 0, "Cannot shutdown client")
  if (helper_bool == false) {
	cmd_unknown(argv[0], argv[1]);
  }
}
