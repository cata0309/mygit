#
# Created by Marincia Cătălin on 15.12.2020.
#

cmake_minimum_required(VERSION 3.0)
project(dbInteract C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_executable(server
        source/server/main.c
        source/server/DbOperations.h
        source/server/DbOperations.c
        libs/parson/parson.h
        libs/parson/parson.c
        source/server/RequestHandlers.h
        source/server/RequestHandlers.c
        source/common/Macros.h
        source/common/Transfer.h
        source/common/Transfer.c)
target_link_libraries(server sqlite3)

add_executable(client
        source/client/main.c
        libs/parson/parson.h
        libs/parson/parson.c
        source/common/Macros.h
        source/common/Transfer.h
        source/common/Transfer.c
        )