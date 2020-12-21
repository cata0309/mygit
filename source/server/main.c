//
// Created by Marincia Cătălin on 15.12.2020.
//

#include <string.h>
#include <sys/stat.h>
#include "RequestHandlers.h"

bool setup_server_socket(i32 *server_socket_fd, char *ip_address, u16 port) {
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  CHECKRET(inet_pton(AF_INET, ip_address, &server_address.sin_addr.s_addr) == 1, false,
		   "Cannot convert str to valid address");
  *server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECKRET(*server_socket_fd != -1, false, "Cannot create socket socket descriptor");
  i32 on = 1;
  CHECKRET(setsockopt(*server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != -1, false,
		   "Cannot set SO_REUSEADDR option to socket");
  CHECKRET(setsockopt(*server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) != -1, false,
		   "Cannot set SO_REUSEPORT option to socket");
  CHECKRET(bind(*server_socket_fd, (const struct sockaddr *)&server_address, sizeof(server_address)) != -1, false,
		   "Cannot bind address to socket socket descriptor");
  return true;
}

i32 main(i32 argc, char **argv) {
  CHECKEXIT(argc >= 3, "Usage: .%s <ip_address> <port> [<backlog>]", strrchr(argv[0], '/'));
  struct stat st;
  if (stat("database.db", &st) == -1) {
	// the file does not exist
	CHECKEXIT(create_database("database.db") == true, false,
			  "Cannot create database")
  }
  char *pointer_end;
  u16 port = (u16)strtol(argv[2], &pointer_end, 10);
  CHECKEXIT(port != 0, "Cannot convert str to unsigned long");
  i32 server_socket_fd;
  CHECKEXIT(setup_server_socket(&server_socket_fd, argv[1], port) == 0, "Cannot setup server server socket");
  u32 backlog = 5;
  if (argc == 4) {
	backlog = (u32)strtol(argv[3], &pointer_end, 10);
	CHECKEXIT(backlog != 0, "Cannot convert str to unsigned long");
  }
  CHECKEXIT(listen(server_socket_fd, backlog) != -1, "Cannot put socket into listening mode");
  i32 clientSockFd;
  pid_t pid;
  while (true) {
	clientSockFd = accept(server_socket_fd, NULL, NULL);
	if (clientSockFd == -1) {
	  continue;
	}
	pid = fork();
	CHECKEXIT (pid != -1, false, "Error at fork()")
	if (pid == 0) {
	  // child
	  char *request = (char*)(malloc(MB20));
	  if (handleClient(clientSockFd) == 0, "Error at handling the client");
	  CHECKEXIT(close(clientSockFd) != -1, "Error at closing client socket descriptor");
	  free(request);
	  // exited
	  exit(EXIT_SUCCESS);
	}
	CHECKEXIT(close(clientSockFd) != -1, "Error at closing client socket descriptor");
  }
  return 0;
}