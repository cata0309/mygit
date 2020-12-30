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
void push_files(const char *working_directory, JSON_Object *request_object);
bool is_natural_number(const char *str);
void serv_conf_send(i32 argc, char **argv);
void init_send(i32 argc, char **argv);
void reset_send(i32 argc, char **argv);
void stage_file_send(i32 argc, char **argv);
void unstage_file_send(i32 argc, char **argv);
void delete_file_send(i32 argc, char **argv);
void restore_file_send(i32 argc, char **argv);
void append_to_changelog_send(i32 argc, char **argv);
void list_dirty_send(i32 argc, char **argv);
void list_untouched_send(i32 argc, char **argv);
void list_staged_send(i32 argc, char **argv);
bool parse_non_connection_command_line(i32 argc, char **argv);
void write_files_to_disk(JSON_Object *json_response_object, const char *repo_directory);
void list_remote_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void clone_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void pull_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void diff_file_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void diff_version_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void checkout_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void checkout_file_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void get_changelog_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void switches_edit_access_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void privacy_switch_send(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *json_request_value);
void push_send(i32 server_socket_fd, i32 argc, JSON_Value *json_request_value);
void register_send(i32 server_socket_fd, i32 argc, JSON_Value *json_request_value);
bool parse_connection_command_line(i32 server_socket_fd, i32 argc, char **argv);
void parse_command_line(i32 argc, char **argv);
#endif //DBINTERACT_SOURCE_CLIENT_REQUESTPREPARE_H_
