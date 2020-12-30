//
// Created by Marincia Cătălin on 15.12.2020.
//

#include "DatabaseInteraction.h"

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
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
  }, "Cannot open database")
  // we create the `USERS` table
  char *statement = "DROP TABLE IF EXISTS USERS; CREATE TABLE USERS(username TEXT PRIMARY KEY, PASSWORD TEXT);";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'USERS' table")
// we create the `REPOSITORY` table
  statement =
	  "DROP TABLE IF EXISTS REPOSITORY;CREATE TABLE REPOSITORY(repository_name TEXT PRIMARY KEY, username TEXT, unix_date INTEGER, is_public INTEGER);";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'REPOSITORY' table")
// we create the `VERSIONS` table
  statement = "DROP TABLE IF EXISTS VERSIONS;CREATE TABLE VERSIONS(repository_name TEXT, author_username TEXT, version"
				  " INTEGER, unix_date INTEGER, changelog TEXT, PRIMARY KEY(repository_name, author_username, version));";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'VERSIONS' table")
// we create the `STORAGE` table
  statement = "DROP TABLE IF EXISTS STORAGE;CREATE TABLE STORAGE(repository_name TEXT, version INTEGER, filename TEXT, "
				  "content TEXT, difference TEXT, PRIMARY KEY(repository_name, version, filename));";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'STORAGE' table")
// we create the `PERMISSIONS` table
  statement = "DROP TABLE IF EXISTS PERMISSIONS;"
				  "CREATE TABLE PERMISSIONS(repository_name TEXT, username TEXT, PRIMARY KEY"
				  "(repository_name, username));";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'PERMISSIONS' table")
  statement = "DROP TABLE IF EXISTS ACCESS;"
				  "CREATE TABLE ACCESS(repository_name TEXT, username TEXT, PRIMARY KEY""(repository_name, username));";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'ACCESS' table")
  statement =
	  "DROP TABLE IF EXISTS DELETIONS;CREATE TABLE DELETIONS(repository_name TEXT, version INTEGER, filename TEXT, PRIMARY ""KEY(version, filename));";
  CHECKCODERET(run_non_select_sqlite_statement(sqlite3_descriptor, statement) == true, false,
			   {
				 CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
			   }, "Cannot create 'DELETIONS' table")
  CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close database")
  return true;
}

bool run_non_select_sqlite_statement(sqlite3 *sqlite3_descriptor, const char *stmt_string)
/* This function is used to execute a specific sql statement on a database rq_main_distributor
 * */
{
  char *error = NULL;
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, stmt_string, 0, 0, &error) == SQLITE_OK, false, { sqlite3_free(error); }, "Error %s", error)
  return true;
}

// adders
bool db_add_user(sqlite3 *sqlite3_descriptor, const char *username, const char *password) {
  char statement[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN);
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  CHECKRET(strlen(password) <= MAX_PASSWORD_LEN, false, "Password too big")
  CHECKRET(sprintf(statement, "INSERT INTO USERS VALUES('%s', '%s');", username, password) > 0, false, "Error at 'sprintf()'")
  bool exists_user = false;
  CHECKRET(db_exists_user(sqlite3_descriptor, username, &exists_user) == true, false, "Error cannot check existence of user")
  if (exists_user == false) {
	return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
  }
  return false;
}

bool db_allow_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  bool can_edit = false;
  CHECKRET(db_exists_edit(sqlite3_descriptor, repository_name, username, &can_edit) == true, false, "Cannot check user permissions")
  if (can_edit == true) { return true; }
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(statement, "INSERT INTO PERMISSIONS VALUES('%s', '%s');", repository_name, username) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool db_allow_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(username) <= MAX_USER_NAME_LEN, false, "Username too big")
  bool can_edit = false;
  CHECKRET(db_exists_access(sqlite3_descriptor, repository_name, username, &can_edit) == true, false, "Cannot check user access")
  if (can_edit == true) { return true; }
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(statement, "INSERT INTO ACCESS VALUES('%s', '%s');", repository_name, username) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool db_insert_version(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *author_username, u16 version, u64 unix_date,
					   const char *changelog) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(statement, "INSERT INTO VERSIONS VALUES('%s', '%s', %d, %lu, '%s');", repository_name, author_username, version, unix_date,
				   changelog) > 0, false, "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool db_mark_as_deleted(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  CHECKRET(strlen(filename) <= MAX_FILE_PATH_LEN, false, "Filename too big")
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET(sprintf(statement, "INSERT INTO DELETIONS VALUES('%s', %d, '%s');", repository_name, version, filename) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool db_insert_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename,
					const char *content, const char *difference) {
  char *statement = (char *)(malloc(MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN + MB5 + MB5));
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN + MB5 + MB5);
  CHECKCODERET(
	  sprintf(statement, "INSERT INTO STORAGE VALUES('%s', %d, '%s', '%s', '%s');", repository_name, version, filename, content, difference) > 0,
	  false, { free(statement); }, "Error at 'sprintf()'")
  bool status = run_non_select_sqlite_statement(sqlite3_descriptor, statement);
  free(statement);
  return status;
}

bool db_insert_repo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, u64 unix_date) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(statement, "INSERT INTO REPOSITORY VALUES('%s', '%s', %lu, %d);", repository_name, username, unix_date, 0) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

// removers
bool db_block_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  bool can_edit = false;
  CHECKRET(db_exists_edit(sqlite3_descriptor, repository_name, username, &can_edit) == true, false, "Cannot check user permissions")
  if (can_edit == false) { return true; }
  char statement[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(statement, "DELETE FROM PERMISSIONS WHERE repository_name='%s' AND username='%s';", repository_name, username) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool db_block_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username) {
  bool can_edit = false;
  CHECKRET(db_exists_access(sqlite3_descriptor, repository_name, username, &can_edit) == true, false, "Cannot check user access")
  if (can_edit == false) { return true; }
  char statement[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(statement, "DELETE FROM ACCESS WHERE repository_name='%s' AND username='%s';", repository_name, username) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

// checkers
bool db_exists_user(sqlite3 *sqlite3_descriptor, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "username='%s'", username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "USERS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool db_exist_credentials(sqlite3 *sqlite3_descriptor, const char *username, const char *password, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN + MAX_PASSWORD_LEN);
  CHECKRET(sprintf(condition, "username='%s' AND password='%s'", username, password) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "USERS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool db_exists_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND version=%d", repository_name, version) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "VERSIONS", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool db_exists_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND version=%d AND filename='%s'", repository_name, version, filename) > 0, false,
		   "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "STORAGE", condition, &count) == true, false, "Cannot get count")
  *context = count > 0;
  return true;
}

bool db_exists_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "PERMISSIONS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool db_exists_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "ACCESS", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}

bool db_exists_repo_in_repository(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool db_exists_repo_in_versions(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "VERSIONS", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool db_exists_repo_in_storage(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s'", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "STORAGE", condition, &count) == true, false, "Failed to get count")
  *context = count > 0;
  return true;
}

bool db_user_owns_repository(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND username='%s'", repository_name, username) > 0, false, "Error at 'sprintf()'")
  u32 count = 0u;
  CHECKRET(db_get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Error cannot get count")
  *context = (count > 0u);
  return true;
}

// getters
bool db_get_count(sqlite3 *sqlite3_descriptor, const char *table_name, const char *condition, u32 *context)
/* This function is used to query the number of rows that match a specific pattern
 * */
{
  char *statement = (char *)malloc(MAX_SQL_STMT_LEN + MAX_TABLE_NAME_LEN + strlen(condition));
  bzero(statement, MAX_SQL_STMT_LEN + MAX_TABLE_NAME_LEN + strlen(condition));
  CHECKCODERET(sprintf(statement, "SELECT COUNT(*) FROM %s WHERE %s;", table_name, condition) > 0, false, { free(statement); },
			   "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKCODERET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, { free(statement); },
			   "Error at preparing statement")
  CHECKCODERET(sqlite3_step(sql_statement) == SQLITE_ROW || sqlite3_step(sql_statement) == SQLITE_DONE, false, { free(statement); },
			   "No rows returned or an error occurred")
  *context = sqlite3_column_int(sql_statement, 0); // this is safe because we always have a non negative value received from
  // COUNT(*)
  free(statement);
  return true;
}

bool db_get_latest_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(statement, "SELECT version FROM versions WHERE repository_name='%s' ORDER BY version DESC LIMIT 1;", repository_name) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(sql_statement);
  CHECKRET(status == SQLITE_DONE || status == SQLITE_ROW, false, "No rows returned or an error occurred")
  if (status == SQLITE_DONE) {
	*context = 0;
  } else {
	*context = sqlite3_column_int(sql_statement, 0); // this is safe because we never insert negative values and never insert
  }
  return true;
}

bool db_get_next_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context) {
  u16 version = 0;
  CHECKRET(db_get_latest_version(sqlite3_descriptor, repository_name, &version) == true, false, "Error at getting the current version")
  CHECKRET(version < USHRT_MAX, false, "Already at the maximum version, cannot increment")
  *context = version + 1;
  return true;
}

bool db_get_changelog(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, char *context) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(statement, "SELECT changelog FROM versions WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(sql_statement);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(sql_statement, 0));
  return true;
}

bool db_get_push_unix_date(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, u32 *context) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  CHECKRET(sprintf(statement, "SELECT unix_date FROM versions WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0, false,
		   "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(sql_statement);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  *context = (u32)sqlite3_column_int(sql_statement, 0);
  return true;
}

bool db_get_file_content(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET (sprintf(statement, "SELECT content FROM STORAGE WHERE repository_name='%s' AND version=%d AND filename='%s';", repository_name,
					version, filename) > 0, false, "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(sql_statement);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(sql_statement, 0));
  return true;
}

bool db_get_file_diff(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename, char *context) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN + MAX_FILE_PATH_LEN);
  CHECKRET (sprintf(statement, "SELECT difference FROM STORAGE WHERE repository_name='%s' AND version=%d AND filename='%s';", repository_name,
					version, filename) > 0, false, "Error at 'sprintf()'")
  sqlite3_stmt *sql_statement;
  CHECKRET(sqlite3_prepare_v2(sqlite3_descriptor, statement, -1, &sql_statement, NULL) == SQLITE_OK, false, "Error at preparing statement")
  i32 status = sqlite3_step(sql_statement);
  CHECKRET(status == SQLITE_ROW, false, "No rows returned or an error occurred")
  strcpy(context, (const char *)sqlite3_column_text(sql_statement, 0));
  return true;
}

i32 inserter(void *json_array_void, i32 number_of_columns, char **column_values, char **column_names) {
  (void) number_of_columns;
  (void) column_names;
  JSON_Array *json_array = (JSON_Array *)(json_array_void);
  json_array_append_string(json_array, column_values[0]);
  return 0;
}

bool db_get_list_of_files(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  char *error = NULL;
  CHECKRET(sprintf(statement, "SELECT filename FROM STORAGE WHERE repository_name='%s' AND version=%d;",
				   repository_name, version) > 0, false, "Error at 'sprintf()'")
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, statement, inserter, json_array, &error) == SQLITE_OK, false, {
	fprintf(stderr, "%s", error);
	sqlite3_free(statement);
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
  }, "Cannot execute operation over rows")
  return true;
}

bool db_get_list_of_deleted_files(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, JSON_Array *json_array) {
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN + UNPREDICTED_LEN);
  char *error = NULL;
  CHECKRET(sprintf(statement, "SELECT filename FROM DELETIONS WHERE repository_name='%s' AND version=%d;", repository_name, version) > 0,
		   false,
		   "Error at 'sprintf()'")
  CHECKCODERET(sqlite3_exec(sqlite3_descriptor, statement, inserter, json_array, &error) == SQLITE_OK, false, {
	fprintf(stderr, "%s", error);
	sqlite3_free(statement);
	CHECKRET(sqlite3_close(sqlite3_descriptor) == 0, false, "Cannot close sqlite rq_main_distributor")
  }, "Cannot execute operation over rows")
  return true;
}

bool db_switch_repository_access(sqlite3 *sqlite3_descriptor, const char *repository_name, bool make_public) {
  CHECKRET(strlen(repository_name) <= MAX_REPO_NAME_LEN, false, "Repository name too big")
  char statement[MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN];
  bzero(statement, MAX_SQL_STMT_LEN + MAX_REPO_NAME_LEN);
  CHECKRET(sprintf(statement, "UPDATE REPOSITORY SET is_public=%d WHERE repository_name='%s';", make_public, repository_name) > 0, false,
		   "Error at 'sprintf()'")
  return run_non_select_sqlite_statement(sqlite3_descriptor, statement);
}

bool is_repo_public(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context) {
  char condition[MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN];
  bzero(condition, MAX_SQL_STMT_LEN + MAX_USER_NAME_LEN);
  CHECKRET(sprintf(condition, "repository_name='%s' AND is_public=1", repository_name) > 0, false, "Error at 'sprintf()'")
  u32 count = 0;
  CHECKRET(db_get_count(sqlite3_descriptor, "REPOSITORY", condition, &count) == true, false, "Error cannot get count")
  *context = count > 0;
  return true;
}
