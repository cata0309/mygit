//
// Created by Marincia Cătălin on 23.12.2020.
//

#ifndef DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
#define DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
#include "../common/Macros.h"
#include "../../libs/parson/parson.h"
#include <stdbool.h>
bool parse_non_connection_command_line(i32 argc, char **argv);
void get_password_from_stdin(bool confirm, char *password);
void get_username_from_stdin(char *username);
void get_credentials(char *username, char *password);
void parse_command_line(i32 argc, char **argv);
void show_help(const char *executable, bool exit_after_show);
#endif //DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
