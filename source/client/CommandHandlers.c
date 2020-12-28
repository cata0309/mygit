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
  CHECKCLIENT(tcgetattr(STDIN_FILENO, &old_settings) != -1, "Error at 'tcgetattr()'")
  new_settings = old_settings;
  new_settings.c_lflag &= ~((u32)ECHO);
  CHECKCLIENT(tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) != -1, "Error at 'tcsetattr()'")
  i32 bytes_read;
  CHECKCLIENT((bytes_read = read(STDIN_FILENO, password, MAX_PASSWORD_LEN)) != -1,
			  "Error at reading the password from stdin")
  password[bytes_read] = '\0';
  for (i32 i = bytes_read; i >= 0; --i) {
	if (password[i] == '\n') {
	  password[i] = '\0';
	  break;
	}
  }
  CHECKCLIENT(tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) != -1, "Error at 'tcsetattr()'")
  printf("\n");
}

void get_username_from_stdin(char *username) {
  fflush(stdout);
  printf("Username: ");
  fflush(stdout);
  i32 bytes_read;
  CHECKCLIENT((bytes_read = read(STDIN_FILENO, username, MAX_USER_NAME_LEN)) != -1,
			  "Error at reading the username from stdin")
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
bool is_repo_public_client(i32 server_socket_fd, u32 key, const char *repository_name) {
  char json_char[MAX_CHECK_VISIBILITY_LEN];
  CHECKCLIENT(
	  sprintf(json_char, "{\"message_type\":\"is_repo_public_request\", \"repository_name\":\"%s\"}", repository_name)
		  > 0, "Error at 'sprintf()'")
  CHECKCLIENT(write_data_three_pass(server_socket_fd, json_char, strlen(json_char), key) == true,
			  "Cannot send the request to the server")
  CHECKCLIENT(read_data_three_pass(server_socket_fd, json_char, key) == true,
			  "Cannot read the response from the server")
  JSON_Value *json_value = json_parse_string(json_char);
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
	  if (!(ip_address[index] >= '0' && ip_address[index] <= '9')) {
		return false;
	  }
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
	CHECKCLIENT(write_with_retry(destination_fd, buffer, bytes_read) != bytes_read,
				"Internal error at writing in staging area")
  }
}

void show_help(const char *executable, bool exit_after_show) {
  printf("Usage : ./%s OPTION [ARGS,...]\nThese are common CMA commands used in various situations:\n"
		 "	help - shows this message\n"
		 "	init <repository_name> - creates a repository with the provided name\n"
		 "	reset - resets the working directory files to the ones from the cloned version\n"
		 "	stage-file <filename> - adds file to the staging area\n"
		 "	unstage-file <filename> - removes file from the staging area\n"
		 "	delete-file <filename> - removes file from the staging area, working directory and untouched directory\n"
		 "	restore-file <filename> - restores the file in the working directory\n"
		 "	append-changelog - appends a message to the current repository changelog\n"
		 "	list-dirty - lists the files from the working directory that are modified\n"
		 "	list-untouched - lists the files from the version of the repository that was cloned\n"
		 "	list-staged - lists the files from the staging area\n"
		 "	list-remote [-v <version>] - lists the files from remote repository, if no version is provided the latest one is "
		 "considered\n"
		 "	register <username> <password> - creates a new account on the server\n"
		 "	clone -n <repository-name> [-v <version>] - clones the remote specified repo, if no version is provided the latest one"
		 " is considered\n"
		 "	pull - gets latest remote version of the current repository\n"
		 "	diff-file-version -f <filename> -v version - gets the differences of current working directory file and one from the "
		 "server at specific version\n"
		 "	diff-version <version> - gets all the differences between the specified version and the one before it\n"
		 "	checkout - gets the latest version of the repository\n"
		 "	checkout-file <filename> [-v version] - gets the file from the repository from the specific version, if none is "
		 "provided the latest one is considered\n"
		 "	get-changelog <version> - gets the changelog of the current repository at specific version\n"
		 "	allow-access <username> - allows access to the repository for a specific user if the repository is private\n"
		 "	block-access <username> - blocks access to the repository for a specific user if the repository is private\n"
		 "	allow-edit <username> - allows edit rights to a specific user for the current repository\n"
		 "	block-edit <username> - removes edit rights of a specific user for the current repository\n"
		 "	go-public - makes the repository public(only the owner can do this)\n"
		 "	go-private - makes the repository private(only the owner can do this)\n"
		 "	push - upload files from staging area to the server\n", executable);
  if (exit_after_show == true) {
	exit(EXIT_SUCCESS);
  }
}

bool is_internal_name(const char *filename) {
  return (strcmp(filename, ".staged") == 0 || strcmp(filename, ".untouched") == 0 || strcmp(filename, ".repo_config") == 0
	  || strcmp(filename, ".marked_as_deleted") == 0 || strcmp(filename, ".changelog") == 0);
}

void show_unknown_command(const char *executable, const char *command) {
  fprintf(stderr, "Unknown command: '%s'\n", command);
  show_help(executable, true);
}

void list_files(const char *directory) {
  DIR *dir = NULL;
  struct dirent *de;
  struct stat st;
  CHECKCLIENT(stat(directory, &st) == 0, "Error at stat")
  CHECKCLIENT (NULL != (dir = opendir(directory)), "Error at opening directory")
  while (NULL != (de = readdir(dir))) {
	if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
	  if (S_ISDIR(st.st_mode) != 0) {
		if (is_internal_name(de->d_name) == false) {
		  printf("- %s\n", de->d_name);
		}
	  }
	}
  }
  CHECKCLIENT(closedir(dir) == 0, "Error at closing directory")
}

bool parse_non_connection_command_line(i32 argc, char **argv) {
  char *option = argv[1];
  char temp_path[PATH_MAX];
  char cwd[PATH_MAX];
  struct stat st;
  if (strcmp(option, "help") == 0) {
	show_help(argv[0], false);
	return true;
  }
  if (strcmp(option, "serv-conf") == 0) {
	char serv_conf_file[MAX_FILE_PATH_LEN];
	char *pointer_end;
	struct passwd *pw = getpwuid(getuid());
	strcpy(serv_conf_file, pw->pw_dir);
	strcat(serv_conf_file, "/.cma_server");
	CHECKCLIENT(argc == 4, "Usage: %s serv-conf ip_address port", argv[0])
	char *ip_address = argv[2];
	CHECKCLIENT(validate_address(ip_address) == true, "You need to insert a valid ip address")
	u16 port = (u16)strtol(argv[3], &pointer_end, 10);
	JSON_Value *conf_value = json_value_init_object();
	JSON_Object *conf_object = json_value_get_object(conf_value);
	json_object_set_string(conf_object, "ip_address", ip_address);
	json_object_set_number(conf_object, "port", port);
	json_serialize_to_file_pretty(conf_value, serv_conf_file);
	return true;
  }
  if (strcmp(option, "init") == 0) {
	CHECKCLIENT(argc == 2, "'%s' is a no arguments option, try again", option)
	CHECKCLIENT(stat(".repo_config", &st) == -1, "There is already a repository created here, you cannot create other one")
	CHECKCLIENT(getcwd(cwd, sizeof(cwd)) != NULL, "Cannot get the current directory path")
	JSON_Value *conf_value = json_value_init_object();
	JSON_Object *conf_object = json_value_get_object(conf_value);
	json_object_set_string(conf_object, "repository_name", strrchr(cwd, '/') + 1);
	json_serialize_to_file_pretty(conf_value, ".repo_config");
	CHECKCLIENT(mkdir(".staging", 0700) == 0, "Cannot create .staging directory")
	CHECKCLIENT(mkdir(".untouched", 0700) == 0, "Cannot create .untouched directory")
	return true;
  }
  CHECKCLIENT(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")
  if (strcmp(option, "reset") == 0) {
	CHECKCLIENT(argc == 2, "'%s' is a no arguments option, try again", option)
	return true;
  }
  if (strcmp(option, "stage-file") == 0) {
	CHECKCLIENT(argc == 3, "Usage: '%s' stage-file <filename>", option)
	CHECKCLIENT(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCLIENT(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCLIENT(stat(argv[2], &st) != -1, "The file '%s' does not exist, cannot add", argv[2])
	CHECKCLIENT(S_ISDIR(st.st_mode) != 0, "You cannot add directories")
	CHECKCLIENT(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	i32 source_fd = open(argv[2], O_RDONLY);
	CHECKCLIENT(source_fd != -1, "Cannot open file for reading '%s'", argv[2])
	strcpy(temp_path, ".staged/");
	strcat(temp_path, argv[2]);
	i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCLIENT(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	copy_content_fd(source_fd, destination_fd);
	CHECKCLIENT(close(source_fd) == 0, "Error at closing source fd for file %s", argv[2])
	CHECKCLIENT(close(destination_fd) == 0, "Error at closing destination fd for file %s", argv[2])
	return true;
  }
  if (strcmp(option, "unstage-file") == 0) {
	CHECKCLIENT(argc == 3, "Usage: '%s' unstage-file <filename>", option)
	CHECKCLIENT(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCLIENT(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCLIENT(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	strcpy(temp_path, ".staged/");
	strcat(temp_path, argv[2]);
	CHECKCLIENT(stat(temp_path, &st) != -1, "The file '%s' does not exist in the staging area, cannot unstage", argv[2])
	CHECKCLIENT(remove(temp_path) != -1, "The file '%s' could not be removed from the staging area", temp_path)
	return true;
  }
  if (strcmp(option, "delete-file") == 0) {
	CHECKCLIENT(argc == 3, "Usage: '%s' delete-file <filename>", option)
	CHECKCLIENT(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCLIENT(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCLIENT(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	CHECKCLIENT(stat(argv[2], &st) != -1, "The file '%s' does not exist, cannot delete", argv[2])
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
	CHECKCLIENT(remove(argv[2]) != -1, "The file '%s' could not be removed from the current working directory", argv[2])
	strcpy(temp_path, ".staged/");
	strcat(temp_path, argv[2]);
	if (stat(temp_path, &st) != -1) {
	  CHECKCLIENT(remove(temp_path) != -1, "The file '%s' could not be removed from the staging area", temp_path)
	}
	return true;
  }
  if (strcmp(option, "restore-file") == 0) {
	CHECKCLIENT(argc == 3, "Usage: '%s' restore-file <filename>", option)
	CHECKCLIENT(!strchr(argv[2], '/'), "You can use only local files")
	CHECKCLIENT(!is_internal_name(argv[2]), "You cannot operate with tool files")
	CHECKCLIENT(!(argv[2][0] == '.' && argv[2][1] == '/'), "There is no sense in using './' in filenames, use the filename")
	strcpy(temp_path, ".untouched/");
	strcat(temp_path, argv[2]);
	CHECKCLIENT(stat(temp_path, &st) != -1,
				"The file '%s' does not exist in the .untouched directory, cannot restore",
				argv[2])
	i32 source_fd = open(temp_path, O_RDONLY);
	i32 destination_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	CHECKCLIENT(source_fd != -1, "Cannot open file for reading '%s'", temp_path)
	CHECKCLIENT(destination_fd != -1, "Cannot open file for writing '%s'", argv[2])
	copy_content_fd(source_fd, destination_fd);
	CHECKCLIENT(close(source_fd) == 0, "Error at closing source fd for file %s", argv[2])
	CHECKCLIENT(close(destination_fd) == 0, "Error at closing destination fd for file %s", argv[2])
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
	CHECKCLIENT(argc == 3, "Usage: '%s' append-to-changelog <message>", option)
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
	CHECKCLIENT(argc == 2, "'%s' is a no arguments option, try again", option)
	CHECKCLIENT(getcwd(cwd, sizeof(cwd)) != NULL, "Cannot get the current directory path")
	list_files(cwd);
	return true;
  }
  if (strcmp(option, "list-untouched") == 0) {
	CHECKCLIENT(argc == 2, "'%s' is a no arguments option, try again", option)
	CHECKCLIENT(getcwd(cwd, sizeof(cwd)) != NULL, "Cannot get the current directory path")
	strcat(cwd, "/.untouched");
	list_files(cwd);
	return true;
  }
  if (strcmp(option, "list-staged") == 0) {
	CHECKCLIENT(argc == 2, "'%s' is a no arguments option, try again", option)
	CHECKCLIENT(getcwd(cwd, sizeof(cwd)) != NULL, "Cannot get the current directory path")
	strcat(cwd, "/.staged");
	list_files(cwd);
	return true;
  }
  return false;
}

bool parse_connection_command_line(i32 server_socket_fd, i32 argc, char **argv) {
  char *option = argv[1];

  JSON_Value *json_request_value = json_value_init_object();
  JSON_Object *json_request_object = json_value_get_object(json_request_value);
  JSON_Value *json_response_value = NULL;
  JSON_Object *json_response_object = NULL;
  bool is_error;
  CHECKCLIENT(argc >= 2, "You must specify at least one option, chose help to see what you can do")
  char *response = (char *)(malloc(MB20));
  u32 client_key = generate_random_key();
  char username[MAX_USER_NAME_LEN];
  char password[MAX_PASSWORD_LEN];
  if (strcmp(option, "register") == 0) {
	CHECKCLIENT(argc == 2, "register is a no arguments option, try again")
	char password_confirm[MAX_PASSWORD_LEN];
	json_object_set_string(json_request_object, "message_type", "add_user_request");
	get_credentials(username, password);
	get_password_from_stdin(true, password_confirm);
	CHECKCLIENT(strcmp(password, password_confirm) == 0, "The passwords do not match, try again")
	json_object_set_string(json_request_object, "username", username);
	json_object_set_string(json_request_object, "password", password);
	CHECKEXIT(write_data_three_pass(server_socket_fd, json_serialize_to_string(json_request_value),
									json_serialization_size(json_request_value), client_key) == true,
			  "Cannot send the credentials to the server")
	CHECKEXIT(read_data_three_pass(server_socket_fd, response, client_key) == true, "Cannot receive response from server")
	// it is ok to exit without deallocating because we terminate the program
	json_response_value = json_parse_string(response);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return;
  }
  JSON_Value *conf_value = json_parse_file(".repo_config");
  JSON_Object *conf_object = json_value_get_object(conf_value);
  const char *repository_name = json_object_dotget_string(conf_object, "repository_name");
  // get repository name
  bool got_already_the_credentials = false;

  if (is_repo_public_client(server_socket_fd, client_key, repository_name) == false) {
	get_credentials(username, password);
	got_already_the_credentials = true;
  }

  if (strcmp(option, "list-remote") == 0) {
	return;

  }
  if (strcmp(option, "clone") == 0) {
	return;

  }
  if (strcmp(option, "pull") == 0) {
	return;

  }

  if (strcmp(option, "diff-file-version") == 0) {
	return;

  }
  if (strcmp(option, "diff-version") == 0) {
	return;

  }
  if (strcmp(option, "checkout") == 0) {
	return;

  }
  if (strcmp(option, "checkout-file") == 0) {
	return;

  }

  if (strcmp(option, "get-changelog") == 0) {
	return;
  }

  if (got_already_the_credentials == false) {
	get_credentials(username, password);
  }
  // this will always require credentials no matter the access specifier of the repository
  if (strcmp(option, "allow-access") == 0 || strcmp(option, "block-access") == 0 || strcmp(option, "allow-edit") == 0
	  || strcmp(option, "block-edit") == 0) {
	CHECKCLIENT(argc == 3, "Usage: %s %s other_username", argv[0], argv[1])
	if (strcmp(option, "allow") == 0) {
	  json_object_set_string(json_request_object, "message_type", "make_repo_public_request");
	} else if (strcmp(option, "block-access") == 0) {
	}
	if (strcmp(option, "allow-edit") == 0) {
	} else {// block-edit
	  json_object_set_string(json_request_object, "message_type", "make_repo_private_request");
	}
	json_object_set_string(json_request_object, "other_username", argv[2]);
	json_object_set_string(json_request_object, "username", username);
	json_object_set_string(json_request_object, "password", password);
	json_object_set_string(json_request_object, "repository_name", repository_name);
	CHECKCLIENT(write_data_three_pass(server_socket_fd, json_serialize_to_string(json_request_value),
									  json_serialization_size(json_request_value), client_key) == true,
				"Cannot send the credentials to the server")
	CHECKCLIENT(read_data_three_pass(server_socket_fd, response, client_key) == true, "Cannot receive response from server")
	// it is ok to exit without deallocating because we terminate the program
	json_response_value = json_parse_string(response);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return;
  }

  if (strcmp(option, "push") == 0) {
	return;

  }

  if (strcmp(option, "go-public") == 0 || strcmp(option, "go-private") == 0) {
	if (strcmp(option, "go-public") == 0) {
	  json_object_set_string(json_request_object, "message_type", "make_repo_public_request");
	} else {
	  json_object_set_string(json_request_object, "message_type", "make_repo_private_request");
	}
	json_object_set_string(json_request_object, "username", username);
	json_object_set_string(json_request_object, "password", password);
	json_object_set_string(json_request_object, "repository_name", repository_name);
	CHECKCLIENT(write_data_three_pass(server_socket_fd, json_serialize_to_string(json_request_value),
									  json_serialization_size(json_request_value), client_key) == true,
				"Cannot send the credentials to the server")
	CHECKCLIENT(read_data_three_pass(server_socket_fd, response, client_key) == true, "Cannot receive response from server")
	// it is ok to exit without deallocating because we terminate the program
	json_response_value = json_parse_string(response);
	json_response_object = json_value_get_object(json_response_value);
	is_error = json_object_dotget_boolean(json_response_object, "is_error");
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"))
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return;
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
  CHECKCLIENT(stat(serv_conf_file, &st) != -1,
			  "Before doing network related operations you must set the remote server details")
  i32 server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_address;
  JSON_Value *conf_value = json_parse_file(serv_conf_file);
  JSON_Object *conf_object = json_value_get_object(conf_value);
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(json_object_dotget_number(conf_object, "port"));
  CHECKCLIENT(inet_pton(AF_INET, json_object_dotget_string(conf_object, "ip_address"), &server_address.sin_addr.s_addr) == 1,
			  "Cannot convert str to valid address")
  CHECKCLIENT(connect(server_socket_fd, (const struct sockaddr *)&server_address, sizeof(server_address)) == 0,
			  "Cannot connect to the server")
  local_bool = parse_connection_command_line(server_socket_fd, argc, argv);
  CHECKCLIENT(shutdown(server_socket_fd, SHUT_RDWR) == 0, "Cannot shutdown client")
  if (local_bool == false) {
	show_unknown_command(argv[0], argv[1]);
  }
}
