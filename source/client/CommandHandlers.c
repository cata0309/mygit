//
// Created by Marincia Cătălin on 23.12.2020.
//

#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
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
  i32 number;
  u8 dots = 0;
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
		number = atoi(section); //todo replace with strtol
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

void parse_command_line(i32 server_socket_fd, i32 argc, char **argv) {
  char *option = argv[1];
  struct stat st;
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
  // this will not require any login or repository name since they do not any of them
  // no connection required
  if (strcmp(option, "help") == 0) {
	//todo list
	return;
  }
  if (strcmp(option, "init") == 0) {
	u16 port;
	char ip_address[16];
	char repository_name[MAX_REPO_NAME_LEN];
	CHECKCLIENT(stat(".repo_config", &st) == -1, "There is already a repository created here")
	CHECKCLIENT(argc == 8, "Usage: %s init -n repository_name -a ip_address -p port", argv[0])
	u8 options_filled = 0;
	for (u8 index = 2; index < 8; ++index) {
	  if (strcmp(argv[index], "-n") == 0) {
		if (index + 1 < 8) {
		  strcpy(repository_name, argv[index + 1]);
		  index += 1;
		  options_filled += 1;
		}
	  } else if (strcmp(argv[index], "-a") == 0) {
		if (index + 1 < 8) {
		  strcpy(ip_address, argv[index + 1]);
		  index += 1;
		  options_filled += 1;
		}
	  } else if (strcmp(argv[index], "-p") == 0) {
		if (index + 1 < 8) {
		  port = atoi(argv[index + 1]); // todo replace with strtol
		  index += 1;
		  options_filled += 1;
		}
	  }
	}
	CHECKCLIENT(options_filled == 3, "Usage: %s init -n repository_name -a ip_address -p port", argv[0])
	CHECKCLIENT(validate_address(ip_address) == true, "You need to insert a valid ip address")
	JSON_Value *conf_value = json_value_init_object();
	JSON_Object *conf_object = json_value_get_object(conf_value);
	json_object_set_string(conf_object, "repository_name", repository_name);
	json_object_set_string(conf_object, "ip_address", ip_address);
	json_object_set_number(conf_object, "port", port);
	json_serialize_to_file_pretty(conf_value, ".repo_config");
	// format json
	return;
  }
  CHECKCLIENT(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")

  if (strcmp(option, "reset") == 0) {
	CHECKCLIENT(argc == 2, "reset is a no arguments option, try again")
	return;
  }

  if (strcmp(option, "add-file") == 0) {
	return;

  }
  if (strcmp(option, "remove-file") == 0) {

	return;

  }
  if (strcmp(option, "restore-file") == 0) {
	return;

  }
  if (strcmp(option, "append-to-changelog") == 0) {
	return;

  }
  if (strcmp(option, "list-dirty") == 0) {
	return;
  }
  if (strcmp(option, "list-untouched") == 0) {
	return;
  }
  if (strcmp(option, "list-staged") == 0) {
	return;
  }

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
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"));
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
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"));
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
	CHECKCLIENT(is_error == false, "%s", json_object_dotget_string(json_response_object, "message"));
	printf("%s\n", json_object_dotget_string(json_response_object, "message"));
	return;
  }
}
