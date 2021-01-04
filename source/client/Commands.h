//
// Created by Marincia Cătălin on 23.12.2020.
//

#ifndef DBINTERACT_SOURCE_CLIENT_COMMAND_H_
#define DBINTERACT_SOURCE_CLIENT_COMMAND_H_
#include "../common/Macros.h"
#include "../../libs/parson/parson.h"
#include <stdbool.h>

void util_read_password(bool confirm_stage, char *password);
void util_read_username(char *username);
void util_read_credentials(char *username, char *password);
bool util_is_repository_public(i32 server_socket_fd, const char *repository_name);
void util_copy_content_of_fd_to_fd(i32 source_fd, i32 destination_fd);
bool util_is_internal_name(const char *filename);
void util_push_populate(const char *working_directory, JSON_Object *request_object);
bool util_is_natural_number(const char *str);
void util_list_or_delete_files(const char *directory, bool delete_mode);
void util_write_files_to_disk(JSON_Object *json_response_object, const char *repo_directory);
i64 util_is_in_array(JSON_Array *array, const char *str);
bool util_is_connection_cmd(const char *option);
bool util_is_non_connection_cmd(const char *option);
void util_multiple_args_command(void(*command)(const char *), u16 argc, char **argv);
void util_execute_command_on_every_non_tool_file(void(*command)(const char *), const char *directory);

void cmd_help(const char *executable, bool exit_after);
void cmd_unknown(const char *executable, const char *command);
void cmd_serv_conf(i32 argc, char **argv);
void cmd_init(i32 argc, char **argv);
void cmd_reset(i32 argc, char **argv);
void cmd_stage_file(const char *filename);
void cmd_unstage_file(const char *filename);
void cmd_delete_file(const char *filename);
void cmd_restore_file(const char *filename);
void cmd_append_message(i32 argc, char **argv);
void cmd_list_dirty(i32 argc, char **argv);
void cmd_list_untouched(i32 argc, char **argv);
void cmd_list_deleted(i32 argc, char **argv);
void cmd_list_staged(i32 argc, char **argv);
void cmd_list_remote(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_clone(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_pull(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_diff_file(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_diff_version(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_checkout(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_checkout_file(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_get_changelog(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_switch_access_or_edit(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_switch_repository_access(i32 server_socket_fd, i32 argc, char **argv, JSON_Value *request_value);
void cmd_push(i32 server_socket_fd, i32 argc, JSON_Value *request_value);
void cmd_register(i32 server_socket_fd, i32 argc, JSON_Value *request_value);

bool cmd_connection_distributor(i32 server_socket_fd, i32 argc, char **argv);
bool cmd_no_connection_distributor(i32 argc, char **argv);
void cmd_distributor(i32 argc, char **argv);
#endif
