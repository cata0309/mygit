#
# Created by Marincia Cătălin on 15.12.2020.
#

cmake_minimum_required(VERSION 3.0)
project(dbInteract C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_executable(aga-serv
        source/server/main.c
        source/server/DatabaseInteraction.h
        source/server/DatabaseInteraction.c
        libs/parson/parson.h
        libs/parson/parson.c
        source/server/Requests.h
        source/server/Requests.c
        source/common/Macros.h
        source/common/Utils.h
        source/common/Utils.c)
target_link_libraries(aga-serv sqlite3)

add_executable(aga
        source/client/main.c
        libs/parson/parson.h
        libs/parson/parson.c
        source/common/Macros.h
        source/common/Utils.h
        source/common/Utils.c
        source/client/Commands.h
        source/client/Commands.c)