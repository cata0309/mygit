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
void get_credentials(char *username, char *password);
bool is_repo_public_client(i32 server_socket_fd, const char *repository_name);
bool validate_address(const char *ip_address);
void copy_content_fd(i32 source_fd, i32 destination_fd);
void show_help(const char *executable, bool exit_after_show);
bool is_internal_name(const char *filename);
void show_unknown_command(const char *executable, const char *command);
void list_or_delete_only_files(const char *directory, bool delete_mode);
void push_files(const char *directory, JSON_Object *request_object);
bool is_natural_number(const char *str);
bool parse_non_connection_command_line(i32 argc, char **argv);
bool parse_connection_command_line(i32 server_socket_fd, i32 argc, char **argv);
void parse_command_line(i32 argc, char **argv);
#endif //DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
