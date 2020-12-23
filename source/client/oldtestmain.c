//
// Created by Marincia Cătălin on 23.12.2020.
//

//
// Created by Marincia Cătălin on 22.12.2020.
//
#include "../common/Transfer.h"
#include "../../libs/parson/parson.h"
#include<string.h>
#include <unistd.h>
i32 main(i32 argc, char **argv) {
  i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(8080);
  inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
  connect(sockfd, (const struct sockaddr *)&servAddr, sizeof(servAddr));
  u32 key = 64;
  JSON_Value *val = json_parse_file("json.json");
  char buffer[7000];
  strncpy(buffer, json_serialize_to_string(val), json_serialization_size(val));
  buffer[json_serialization_size(val)]='\0';
  printf("trying to send: '%s'\n", buffer);
  write_data_three_pass(sockfd, buffer, strlen(buffer), key);
  char resp[10000];
  read_data_three_pass(sockfd, resp, key);
  printf("received response: '%s'\n", resp);
  shutdown(sockfd, SHUT_RDWR);
  return 0;
}