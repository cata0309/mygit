//
// Created by Marincia Cătălin on 22.12.2020.
//

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include "CommandHandlers.h"
//i32 main(i32 argc, char **argv) {
////  JSON_Value *request_json_value = json_value_init_object();
////  get_credentials(request_json_value);
////  printf("got: '%s'", json_serialize_to_string(request_json_value));
////
//// Created by Marincia Cătălin on 23.12.2020.
////
//
////
//// Created by Marincia Cătălin on 22.12.2020.
////
//#include "../common/Transfer.h"
//#include "../../libs/parson/parson.h"
//#include<string.h>
//#include <unistd.h>

i32 main(i32 argc, char **argv) {
//  struct stat st;
//  JSON_Value *marked_as_deleted_value;
//  JSON_Array *marked_as_deleted_array;
//  if(stat(".marked_as_deleted", &st)!=-1){
//	marked_as_deleted_value=json_parse_file(".marked_as_deleted");
//	marked_as_deleted_array=json_value_get_array(marked_as_deleted_value);
//  } else {
//	marked_as_deleted_value=json_value_init_array();
//	marked_as_deleted_array=json_value_get_array(marked_as_deleted_value);
//  }
//  json_array_append_string(marked_as_deleted_array, "caca");
//  json_array_append_string(marked_as_deleted_array, "caca2");
//  json_serialize_to_file_pretty(marked_as_deleted_value, ".marked_as_deleted");
//  printf("test: '%s'\n", )
  CHECKCLIENT(argc > 1, "You must provide an option, use help to see what is available")
  parse_non_connection_command_line(argc, argv);
//	char*ptr="testare scris in home directory";
//	int bytes_written=write(test_home, ptr, strlen(ptr));
//	printf("%d", bytes_written);
//	close(test_home);
//  i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
//  struct sockaddr_in servAddr;
//  servAddr.sin_family = AF_INET;
//  struct stat st;
//  CHECKCLIENT(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")
//
//  if(stat(".repo_config", &st)=0){}else{
//
//  }
//  connect(sockfd, (const struct sockaddr *)&servAddr, sizeof(servAddr));
//  parse_command_line(sockfd, argc, argv);
//  shutdown(sockfd, SHUT_RDWR);
  return 0;
}
