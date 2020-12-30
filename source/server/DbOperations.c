//
// Created by Marincia Cătălin on 15.12.2020.
//

#include "DbOperations.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
// misc

bool create_database(const char *filename)
/* This function is used to create the database with a specific filename, it includes USERS, REPOSITORY, VERSIONS,
 * STORAGE and PERMISSIONS tables
 * */
{
  // this is the sqlite3_descriptor that will be used to execute SQLite commands on, it's similar to a file descriptor
  sqlite3 *sqlite3_descriptor;
  // we open/create the database file and also associate it to the sqlite3_descriptor
  CHECKCODERET(sqlite3_open(filename, &sqlite3_descriptor) == SQLITE_OK, false, {
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
  }, "Cannot open database")
  // we create the `USERS` table
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor, "DROP TABLE IF EXISTS USERS;"
														  "CREATE TABLE USERS(username TEXT PRIMARY KEY, PASSWORD TEXT);")
		  == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'USERS' table")
// we create the `REPOSITORY` table
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor, "DROP TABLE IF EXISTS REPOSITORY;"
														  "CREATE TABLE REPOSITORY(repository_name TEXT PRIMARY KEY, username TEXT, unix_date "
														  "INTEGER, is_public INTEGER);") == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'REPOSITORY' table")
// we create the `VERSIONS` table
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor, "DROP TABLE IF EXISTS VERSIONS;"
														  "CREATE TABLE VERSIONS(repository_name TEXT, author_username TEXT, version"
														  " INTEGER, unix_date INTEGER, changelog TEXT, PRIMARY KEY(repository_name, author_username, version));")
		  == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'VERSIONS' table")
// we create the `STORAGE` table
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor, "DROP TABLE IF EXISTS STORAGE;"
														  "CREATE TABLE STORAGE(repository_name TEXT, version INTEGER, filename TEXT, "
														  "content TEXT, difference TEXT, PRIMARY KEY(repository_name, "
														  "version, filename));")
		  == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'STORAGE' table")
// we create the `PERMISSIONS` table
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor,
									  "DROP TABLE IF EXISTS PERMISSIONS;"
									  "CREATE TABLE PERMISSIONS(repository_name TEXT, username TEXT, PRIMARY KEY"
									  "(repository_name, username));")
		  == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'PERMISSIONS' table")
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor,
									  "DROP TABLE IF EXISTS ACCESS;"
									  "CREATE TABLE ACCESS(repository_name TEXT, username TEXT, PRIMARY KEY"
									  "(repository_name, username));")
		  == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'ACCESS' table")
  CHECKCODERET(
	  run_non_select_sqlite_statement(sqlite3_descriptor,
									  "DROP TABLE IF EXISTS DELETIONS;"
									  "CREATE TABLE DELETIONS(repository_name TEXT, version INTEGER, filename TEXT, PRIMARY "
									  "KEY(version, filename));") == true,
	  false,
	  {
		CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
	  },
	  "Cannot create 'DELETIONS' table")
  CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close database")
  return true;
}

bool run_non_select_sqlite_statement(sqlite3 *sqlite3_descriptor, const char *stmt_string)
/* This function is used to execute a specific sql statement on a database handler_requests
 * */
{
  char *error_msg = NULL;
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, stmt_string, 0, 0, &error_msg) == SQLITE_OK, false, { sqlite3_free(error_msg); }, "Error %s",
			   error_msg)
  return true;
}

// adders
bool add_user(sqlite3 *sqlite3_descriptor, const char *username, const char *password) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN);
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  CHECKRET(strlen(password) <= MAX_PASSWORD_LEN, false, "Password too big")
  CHECKRET(sprintf(stmt_string, "INSERT INTO USERS VALUES('%s', '%s');", username, password) > 0, false, "Error at 'sprintf()'")
  bool does_user_exist = false;
  CHECKRET(user_exists(sqlite3_descriptor, username, &does_user_exist) == true, false, "Error cannot check existence of user")
  if (does_user_exist == false) {
	return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
  }
  return false;
}

bool add_permission(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  bool has_perms = false;
  CHECKRET(permission_exists(sqlite3_descriptor, repository_name, username, &has_perms) == true, false, "Cannot check user permissions")
  if (has_perms == true) { return true; }
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "INSERT INTO PERMISSIONS VALUES('%s', '%s');", repository_name, username) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool add_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  bool has_perms = false;
  CHECKRET(access_exists(sqlite3_descriptor, repository_name, username, &has_perms) == true, false, "Cannot check user access")
  if (has_perms == true) { return true; }
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "INSERT INTO ACCESS VALUES('%s', '%s');", repository_name, username) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool add_version(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *author_username, u16 version, u64 unix_date,
				 const char *changelog) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(stmt_string, "INSERT INTO VERSIONS VALUES('%s', '%s', %d, %lu, '%s');",
				   repository_name, author_username, version, unix_date, changelog) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool add_deleted_filename(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(filename) <= MAX_FILE_PATH_LEN, false, "Filename too big")
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET(sprintf(stmt_string, "INSERT INTO DELETIONS VALUES('%s', %d, '%s');", repository_name, version, filename) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool add_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename,
			  const char *content, const char *difference) {
  char *stmt_string = (char *)(malloc(MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN + MB5 + MB5));
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN + MB5 + MB5);
  CHECKCODERET(
	  sprintf(stmt_string, "INSERT INTO STORAGE VALUES('%s', %d, '%s', '%s', '%s');", repository_name, version, filename, content, difference) > 0,
	  false, { free(stmt_string); }, "Error at 'sprintf()'")
  bool code = run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
  free(stmt_string);
  return code;
}

bool add_repo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, u64 unix_date) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(stmt_string, "INSERT INTO REPOSITORY VALUES('%s', '%s', %lu, %d);", repository_name, username, unix_date, 0) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

// removers
bool del_permission(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  bool has_perms = false;
  CHECKRET(permission_exists(sqlite3_descriptor, repository_name, username, &has_perms) == true, false, "Cannot check user permissions")
  if (has_perms == false) { return true; }
  char stmt_string[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "DELETE FROM PERMISSIONS WHERE repository_name='%s' AND username='%s';", repository_name, username) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool del_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  bool has_perms = false;
  CHECKRET(access_exists(sqlite3_descriptor, repository_name, username, &has_perms) == true, false, "Cannot check user access")
  if (has_perms == false) { return true; }
  char stmt_string[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "DELETE FROM ACCESS WHERE repository_name='%s' AND username='%s';", repository_name, username) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

// checkers
bool user_exists(sqlite3 *sqlite3_descriptor, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "username='%s'", username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "USERS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool credentials_exist(sqlite3 *sqlite3_descriptor, const char *username, const char *password, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN);
  CHECKRET(sprintf(condition, "username='%s' AND password='%s'", username, password) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "USERS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool version_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND version=%d", repository_name, version) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "VERSIONS", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool file_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND version=%d AND filename='%s'", repository_name, version, filename) > 0, false,
		   "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "STORAGE", condition, &count) == true, false, "Cannot get count")
  *context = count > 0;
  return true;
}

bool permission_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "PERMISSIONS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool access_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "ACCESS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool repo_exists_in_REPOSITORY(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool repo_exists_in_VERSIONS(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "VERSIONS", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool repo_exists_in_STORAGE(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "STORAGE", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool is_the_user_the_creator_of_the_repo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0u;
  CHECKRET(get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Error cannot get count")
  *context = (count > 0u);
  return true;
}

// getters
bool get_count(sqlite3 *sqlite3_descriptor, const char *table_name, const char *condition, u32 *context)
/* This function is used to query the number of rows that match a specific pattern
 * */
{
  char *stmt_string = (char *)malloc(MAX_SQL_STMT_LEN + MAX_TABLE_NAME_LEN + strlen(condition));
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_TABLE_NAME_LEN + strlen(condition));
  CHECKCODERET(sprintf(stmt_string, "SELECT COUNT(*) FROM %s WHERE %s;", table_name, condition) > 0, false, { free(stmt_string); },
			   "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKCODERET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, { free(stmt_string); },
			   "Error at preparing statement")
  CHECKCODERET(sqlite3_step(stmt) == SQLITE_ROW || sqlite3_step(stmt) == SQLITE_DONE, false, { free(stmt_string); },
			   "No rows returned or an error occurred")
  *context = sqlite3_column_int(stmt, 0); // this is safe because we always have a non negative value received from
  // COUNT(*)
  free(stmt_string);
  return true;
}

bool get_current_version_number(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "SELECT version FROM versions WHERE repository_name='%s' ORDER BY version DESC LIMIT 1;", repository_name) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(stmt);
  CHECKRET(status == SQLITE_DONE || status == SQLITE_ROW, false, "No rows returned or an error occurred")
  if (status == SQLITE_DONE) {
	*context = 0;
  } else {
	*context = sqlite3_column_int(stmt, 0); // this is safe because we never insert negative values and never insert
	// values over the USHRT_MAX
  }
  return true;
}

bool get_next_version_number(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context) {
  u16 current_version = 0;
  CHECKRET(get_current_version_number(sqlite3_descriptor, repository_name, &current_version) == true, false, "Error at getting the current version")
  CHECKRET(current_version < USHRT_MAX, false, "Already at the maximum version, cannot increment")
  *context = current_version + 1;
  return true;
}

bool get_changelog_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, char *context) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(stmt_string, "SELECT changelog FROM versions WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(stmt);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(stmt, 0));
  return true;
}

bool get_push_time_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, u32 *context) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(stmt_string, "SELECT unix_date FROM versions WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(stmt);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  *context = (u32)sqlite3_column_int(stmt, 0);
  return true;
}

bool get_file_content(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET (sprintf(stmt_string, "SELECT content FROM STORAGE WHERE repository_name='%s' AND version=%d AND filename='%s';", repository_name,
					version, filename) > 0, false, "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(stmt);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(stmt, 0));
  return true;
}

bool get_file_difference(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET (sprintf(stmt_string, "SELECT difference FROM STORAGE WHERE repository_name='%s' AND version=%d AND filename='%s';", repository_name,
					version, filename) > 0, false, "Error at 'sprintf()'")
  sqlite3_stmt *stmt;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, stmt_string, -1, &stmt, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(stmt);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(stmt, 0));
  return true;
}

i32 inserter(void *json_array_void, i32 number_of_columns, char **column_values, char **column_names) {
  JSON_Array *json_array = (JSON_Array *)(json_array_void);
  json_array_append_string(json_array, column_values[0]);
  return 0;
}

bool get_ls_remote_files_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  char *error_msg = NULL;
  CHECKRET(sprintf(stmt_string, "SELECT filename FROM STORAGE WHERE repository_name='%s' AND version=%d;",
				   repository_name, version) > 0, false, "Error at 'sprintf()'")
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, stmt_string, inserter, json_array, &error_msg) == SQLITE_OK, false, {
	fprintf(stderr, "%s", error_msg);
	sqlite3_free(stmt_string);
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
  }, "Cannot execute operation over rows")
  return true;
}

bool get_ls_deleted_files_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array) {
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  char *error_msg = NULL;
  CHECKRET(sprintf(stmt_string, "SELECT filename FROM DELETIONS WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0,
		   false,
		   "Error at 'sprintf()'")
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, stmt_string, inserter, json_array, &error_msg) == SQLITE_OK, false, {
	fprintf(stderr, "%s", error_msg);
	sqlite3_free(stmt_string);
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite handler_requests")
  }, "Cannot execute operation over rows")
  return true;
}

bool make_repo_public(sqlite3 *sqlite3_descriptor, const char *repository_name) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "UPDATE REPOSITORY SET is_public=1 WHERE repository_name='%s';", repository_name) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}

bool is_repo_public(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND is_public=1", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool make_repo_private(sqlite3 *sqlite3_descriptor, const char *repository_name) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  char stmt_string[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(stmt_string, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(stmt_string, "UPDATE REPOSITORY SET is_public=0 WHERE repository_name='%s';", repository_name) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, stmt_string);
}