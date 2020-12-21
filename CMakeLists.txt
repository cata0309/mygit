cmake_minimum_required(VERSION 3.17)
project(dbInteract C)

set(CMAKE_C_STANDARD 99)

#add_executable(dbInteract main.c)
add_executable(dbInteract main.c DbOperations.h DbOperations.c libs/parson/parson.h libs/parson/parson.c RequestHandlers.h RequestHandlers.c)
target_link_libraries(dbInteract sqlite3)