//
// Created by Marincia Cătălin on 22.12.2020.
//

#include <string.h>
#include <sys/stat.h>
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

  i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  struct stat st;
  CHECKCLIENT(stat(".repo_config", &st) != -1, "You must clone or create a repository before doing operations on it")

  if(stat(".repo_config", &st)=0){}else{

  }
  connect(sockfd, (const struct sockaddr *)&servAddr, sizeof(servAddr));
  parse_command_line(sockfd, argc, argv);
  shutdown(sockfd, SHUT_RDWR);
  return 0;
}
