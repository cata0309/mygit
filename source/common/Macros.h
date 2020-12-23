//
// Created by Marincia Cătălin on 22.12.2020.
//

#ifndef DBINTERACT_SOURCE_COMMON_MACROS_H_
#define DBINTERACT_SOURCE_COMMON_MACROS_H_
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
typedef u_char uchar;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

#define MAX_TABLE_NAME_LEN 20
#define MAX_SQL_STMT_LEN 300
#define MAX_FILE_NAME_LEN 50
#define MAX_REPO_NAME_LEN 40
#define MAX_USER_NAME_LEN 25
#define MAX_PASSWORD_LEN 35
#define MAX_CHECK_VISIBILITY_LEN 400
#define UNPREDICTED_LEN 40
#define MAX_KEY 1024
#define MIN_KEY 10
// MB1
#define MAX_CHANGELOG_LEN 1048576
#define MB5 5242880
#define MB20 20971520
//#define MB15 15728640
#define MB10 10485760

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
#define CHECKCLIENT(condition, ...)\
  if(!(condition)){\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    exit(EXIT_FAILURE);\
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
#endif