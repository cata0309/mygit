//
// Created by Marincia Cătălin on 23.12.2020.
//

#ifndef DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
#define DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
#include "../common/Macros.h"
#include "../../libs/parson/parson.h"
#include <stdbool.h>

void get_password_from_stdin(bool confirm, char *password);
void get_username_from_stdin(char *username);
void get_credentials(char*username, char*password);
void parse_command_line(i32 server_socket_fd, i32 argc, char**argv);
#endif //DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
