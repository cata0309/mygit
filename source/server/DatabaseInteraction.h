//
// Created by Marincia Cătălin on 15.12.2020.
//

#ifndef DBINTERACT_DATABASE_INTERACTION_H_
#define DBINTERACT_DATABASE_INTERACTION_H_
#include "../../libs/parson/parson.h"
#include "../common/Macros.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// misc
bool create_database(const char *filename);
bool run_non_select_sqlite_statement(sqlite3 *sqlite3_descriptor, const char *stmt_string);
// adders
bool db_add_user(sqlite3 *sqlite3_descriptor, const char *username, const char *password);
bool db_allow_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool db_allow_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool db_mark_as_deleted(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename);
bool db_insert_version(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *author_username, u16 version, u64 unix_date,
					   const char *changelog);
bool db_insert_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, const char *content,
					const char *difference);
bool db_insert_repo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, u64 unix_date);
// removers
bool db_block_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool db_block_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
// checkers
bool db_exists_user(sqlite3 *sqlite3_descriptor, const char *username, bool *context);
bool db_exist_credentials(sqlite3 *sqlite3_descriptor, const char *username, const char *password, bool *context);
bool db_exists_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, bool *context);
bool db_exists_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, bool *context);
bool db_exists_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
bool db_exists_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
bool db_exists_repo_in_repository(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool db_exists_repo_in_versions(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool db_exists_repo_in_storage(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool db_user_owns_repository(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
// getters
bool db_get_count(sqlite3 *sqlite3_descriptor, const char *table_name, const char *condition, u32 *context);
bool db_get_latest_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context);
bool db_get_next_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context);
bool db_get_changelog(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, char *context);
bool db_get_push_unix_date(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, u32 *context);
bool db_get_file_content(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context);
bool db_get_file_diff(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context);
bool db_get_list_of_files(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array);
bool db_get_list_of_deleted_files(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array);
bool db_switch_repository_access(sqlite3 *sqlite3_descriptor, const char *repository_name, bool make_public);
bool is_repo_public(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
#endif
