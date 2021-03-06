//
// Created by Marincia Cătălin on 15.12.2020.
//
#include "Requests.h"
#include "../common/Utils.h"

#include <string.h>
#include <sys/stat.h>
#include <wait.h>

bool setup_server_socket(i32 *server_socket_fd, char *ip_address, u16 port) {
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  CHECKRET(inet_pton(AF_INET, ip_address, &server_address.sin_addr.s_addr) == 1, false, "Cannot convert str to valid address")
  *server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECKRET(*server_socket_fd != -1, false, "Cannot create socket socket descriptor")
  i32 on = 1;
  CHECKRET(setsockopt(*server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != -1, false, "Cannot set SO_REUSEADDR option to socket")
  CHECKRET(setsockopt(*server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) != -1, false, "Cannot set SO_REUSEPORT option to socket")
  CHECKRET(bind(*server_socket_fd, (const struct sockaddr *)&server_address, sizeof(server_address)) != -1,
		   false,
		   "Cannot bind address to socket socket descriptor")
  return true;
}

i32 main(i32 argc, char **argv) {
  CHECKEXIT(argc >= 3, "Usage: .%s <ip_address> <port> [<backlog>]", strrchr(argv[0], '/'))
  CHECKEXIT(util_is_address_valid(argv[1]) == true, "You must provide a valid ip address, '%s' is not a valid address", argv[1])
  struct stat st;
  if (stat("database.db", &st) == -1) {
	// the file does not exist
	CHECKEXIT(create_database("database.db") == true, "Cannot create database")
  }
  char *conversion_ptr;
  u16 port = (u16)strtol(argv[2], &conversion_ptr, 10);
  CHECKEXIT(port != 0, "Cannot convert str to unsigned long")
  i32 server_socket_fd;
  CHECKEXIT(setup_server_socket(&server_socket_fd, argv[1], port) == true, "Cannot setup server socket")
  u32 backlog = 5;
  if (argc == 4) {
	backlog = (u32)strtol(argv[3], &conversion_ptr, 10);
	CHECKEXIT(backlog != 0, "Cannot convert str to unsigned long")
  }
  CHECKEXIT(listen(server_socket_fd, backlog) != -1, "Cannot put socket into listening mode")
  sqlite3 *sqlite3_descriptor;
  CHECKEXIT(sqlite3_open("database.db", &sqlite3_descriptor) == SQLITE_OK, "Cannot open database")
  i32 client_sock_fd;
  i32 status;
  struct sockaddr_in client_address;
  u32 len_address;
  pid_t pid;
  bool handler_exit_status;
  while (true) {
	client_sock_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &len_address);
	if (client_sock_fd == -1) {
	  continue;
	}
	pid = fork();
	CHECKEXIT (pid != -1, "Error at fork()")
	if (pid == 0) {
	  // child
	  char *request_buffer = (char *)(malloc(MB20));
	  bzero(request_buffer, MB20);
	  CHECKCODERET(read_with_prefix(client_sock_fd, request_buffer) == true,
				   1,
				   { free(request_buffer); },
				   "Error at reading")
	  JSON_Value *response_value = json_value_init_object();
	  JSON_Object *response_root_object = json_value_get_object(response_value);
	  handler_exit_status = rq_main_distributor(sqlite3_descriptor, request_buffer, response_value);
	  json_object_set_boolean(response_root_object, "is_error", !handler_exit_status);
	  CHECKCODERET(write_with_prefix(client_sock_fd,
									 json_serialize_to_string(response_value),
									 json_serialization_size(response_value)) == true, 1,
				   { free(request_buffer); },
				   "Cannot write response to client")
	  if (strcmp(json_object_dotget_string(response_root_object, "message_type"), "is_repo_public_response") == 0) {
		bzero(request_buffer, MB20);
		json_object_clear(response_root_object);
		CHECKCODERET(read_with_prefix(client_sock_fd, request_buffer) == true,
					 1,
					 { free(request_buffer); },
					 "Error at reading")
		handler_exit_status = rq_main_distributor(sqlite3_descriptor, request_buffer, response_value);
		json_object_set_boolean(response_root_object, "is_error", !handler_exit_status);
		CHECKCODERET(write_with_prefix(client_sock_fd,
									   json_serialize_to_string(response_value),
									   json_serialization_size(response_value)) == true, 1,
					 { free(request_buffer); },
					 "Cannot write response to client")
	  }
	  // exited
	  free(request_buffer);
	  fflush(stdout);
	  return 0;
	} else {
	  CHECKEXIT (waitpid(pid, &status, 0) != -1, "Error at waitpid")
	  if (WIFEXITED(status)) {
		printf("The client from the ip_address: %s:%d handling exited with status %d\n",
			   inet_ntoa(client_address.sin_addr),
			   client_address.sin_port,
			   WEXITSTATUS(status));
	  }
	}
	CHECKEXIT(close(client_sock_fd) != -1, "Error at closing client socket descriptor");
  }
//  return 0;
}