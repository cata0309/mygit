#ifndef DBINTERACT_DBOPERATIONS_H_
#define DBINTERACT_DBOPERATIONS_H_

#include <sqlite3.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "libs/parson/parson.h"

//typedef int8_t i8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef uint64_t u64;
#define MAX_TABLE_NAME_LEN 20
#define MAX_SQL_STMT_LEN 300
#define MAX_FILE_NAME_LEN 50
#define MAX_REPO_NAME_LEN 40
#define MAX_USER_NAME_LEN 25
#define MAX_PASSWORD_LEN  35
#define UNPREDICTED_LEN 40
// MB1
#define MAX_CHANGELOG_LEN 1048576
#define MB5 5242880
//#define MB20 20971520
//#define MB15 15728640
//#define MB10 10485760

//#define MB1 1048576

#define READ_END 0
#define WRITE_END 1

#define CHECKRET(condition, value, ...)\
  if(!(condition)){\
    fprintf(stderr, "pid: %d, line: %d, func: %s, msg: ", getpid(), __LINE__, __func__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    return value;\
  }

#define CHECKCODERET(condition, value, code, ...)\
  if(!(condition)){\
    fprintf(stderr, "pid: %d, line: %d, func: %s, msg: ", getpid(), __LINE__, __func__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");                       \
    code                                          \
    return value;\
  }

#define CHECKEXIT(condition, ...)\
  if(!(condition)){\
    fprintf(stderr, "pid: %d, line: %d, func: %s, msg: ", getpid(), __LINE__, __func__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    exit(EXIT_FAILURE);\
  }

#define CHECKBK(condition, ...)\
  if(!(condition)){\
    fprintf(stderr, "pid: %d, line: %d, func: %s, msg:", getpid(), __LINE__, __func__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    break;\
  }

// misc
bool create_database(const char *filename);
bool run_non_select_sqlite_statement(sqlite3 *sqlite3_descriptor, const char *stmt_string);
// adders
bool add_user(sqlite3 *sqlite3_descriptor, const char *username, const char *password);
bool add_permission(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool add_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool add_deleted_filename(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename);
bool add_version(sqlite3 *sqlite3_descriptor,
				 const char *repository_name,
				 const char *author_username,
				 u16 version,
				 u64 unix_date,
				 const char *changelog);
bool add_file(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename,
			  const char *content, const char *difference);
bool add_repo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, u64 unix_date);
// removers
bool del_permission(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
bool del_access(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username);
// checkers
bool user_exists(sqlite3 *sqlite3_descriptor, const char *username, bool *context);
bool credentials_exist(sqlite3 *sqlite3_descriptor, const char *username, const char *password, bool *context);
bool version_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, bool *context);
bool file_exists(sqlite3 *sqlite3_descriptor,
				 const char *repository_name,
				 u16 version,
				 const char *filename,
				 bool *context);
bool permission_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
bool access_exists(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
bool repo_exists_in_REPOSITORY(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool repo_exists_in_VERSIONS(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool repo_exists_in_STORAGE(sqlite3 *sqlite3_descriptor, const char *repository_name, bool *context);
bool isUserTheCreatorOfRepo(sqlite3 *sqlite3_descriptor, const char *repository_name, const char *username, bool *context);
// getters
bool get_count(sqlite3 *sqlite3_descriptor, const char *table_name, const char *condition, u32 *context);
bool get_current_version_number(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context);
bool get_next_version_number(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 *context);
bool get_changelog_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, char *context);
bool get_file_content(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename,
					  char *context);
bool get_file_difference(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version, const char *filename,
						 char *context);
bool get_ls_remote_files_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version,
								 JSON_Array *json_array);
bool get_ls_deleted_files_version(sqlite3 *sqlite3_descriptor, const char *repository_name, u16 version,
								  JSON_Array *json_array);
bool make_repo_public(sqlite3*sqlite3_descriptor, const char*repository_name);
bool is_repo_public(sqlite3*sqlite3_descriptor, const char*repository_name, bool*context);
bool make_repo_private(sqlite3*sqlite3_descriptor, const char*repository_name);
#endif
