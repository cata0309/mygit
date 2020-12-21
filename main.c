#include <string.h>
#include "RequestHandlers.h"

#define try(condition) \
     printf("status line[%d] {%s}\n", __LINE__, condition?"OK":"FAIL");
int main(int argc, char *argv[]) {
  if(argc>2){
    create_database("database.db");
    printf("yes");
    exit(EXIT_SUCCESS);
  }
//  CHECKEXIT(create_database("database.db")==true, "Cannot create database")
  sqlite3 *sqlite3_descriptor;
  CHECKEXIT(sqlite3_open("database.db", &sqlite3_descriptor) == SQLITE_OK, "Cannot open database")
//
//  try(add_user(sqlite3_descriptor, "catalin", "parola1"))
////  try(add_user(sqlite3_descriptor, "andreea", "parola2"))
//  try(add_user(sqlite3_descripstor, "alexandra", "parola1"))
////  try(add_user(sqlite3_descriptor, "andrei", "parola3"))
//  try(add_user(sqlite3_descriptor, "andrei", "parola1"))
////  try(add_user(sqlite3_descriptor, "george", "parola5"))
//
//	try(add_repo(sqlite3_descriptor, "my-music", "alexandra", 10000))

//	handler_requests("{\"message_type\":\"login\", \"username\": \"catalin\", \"password\": \"nino\"}}");
//	handler_requests("{\"message_type\":\"bananaio\"}");
  JSON_Value *jsvalue = NULL;
//  try(handler_requests(sqlite3_descriptor,
//"{\"message_type\":\"add_permission_request\",\"repository_name\":\"my-music\", \"own_username\":\"alexandra\","
//"\"other_username\":\"andrei\"}", js))
//  try(handler_requests(sqlite3_descriptor,
//			  "{\"message_type\":\"add_user_request\",\"repository_name\":\"my-music\", \"username\":\"alexandra1\","
//			  "\"password\":\"parola12\"}", js))
//  try(handler_requests(sqlite3_descriptor,
//			  "{\"message_type\":\"diff_request\",\"repository_name\":\"my-music\", \"filename\":\"main.cpp\","
//			  "\"version\":2}", js))
//  try(handler_requests(sqlite3_descriptor,
//			  "{\"message_type\":\"diff_request\",\"repository_name\":\"my-music\", \"filename\":\"main.cpp\"}", js))
  jsvalue = json_value_init_object();
//  JSON_Object *jsobject=json_value_get_object(jsvalue);
// json_object_set_number(jsobject, "numar", 15);
// char *name=(char*)(malloc(100));
// strcpy(name, "marincia");
// json_object_set_string(jsobject, "nume", name);
// free(name);
// printf("%d\n", (i32)json_object_dotget_number(jsobject, "numar"));
// printf("%s\n", json_object_dotget_string(jsobject, "nume"));
//
// printf("%d\n", sizeof(JSON_Value*));
// printf("%s\n", json_serialize_to_string(jsvalue));
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"checkout_file_request\", "
//								  "\"repository_name\":\"my-music\",\"has_version\":true, \"version\":12,"
//								  "\"filename\":\"main.cpp\"}",
//			  jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string(jsvalue));
//  printf("size: %zu", json_serialization_size(jsvalue));
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"add_user_request\", "
//								  "\"username\":\"alexandra\",\"password\":\"parola1\"}",
//			  jsvalue))
//    printf("response: '%s'\n", json_serialize_to_string(jsvalue));

//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"login_request\", "
//								  "\"username\":\"catalin\",\"password\":\"parola1\"}",
//			  jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string(jsvalue));

//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"add_permission_request\", "
//								  "\"own_username\":\"alexandra\",\"other_username\":\"george\","
//		  "\"repository_name\":\"my-music\"}",
//			  jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string(jsvalue));
//
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"diff_request\", "
//								  "\"repository_name\":\"proiect la plp\",\"has_version\":true, \"version\":2,"
//								  "\"filename\":\"gigi.c\"}",
//			  jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));

//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"get_changelog_request\", "
//										   "\"repository_name\":\"my-music\",\"has_version\":true, \"version\":12}",
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"get_changelog_request\", "
//										   "\"repository_name\":\"my-music\",\"has_version\":false, \"all_versions\":true}",
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"get_changelog_request\", "
//										   "\"repository_name\":\"my-music\",\"has_version\":false, \"all_versions\":true}",
//					   jsvalue))
//					   investigat, trebuie adaugat mesaj de eroare
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"get_changelog_request\", "
//										   "\"repository_name\":\"proiect la plp\",\"has_version\":false, "
//			 "\"all_versions\":false}",
//					   jsvalue))
////					   investigat, trebuie adaugat mesaj de eroare
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));

//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"ls_remote_files_request\", "
//										   "\"repository_name\":\"proiect la plp\",\"has_version\":true, \"version\":2}",
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));


//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"checkout_request\", "
//										   "\"repository_name\":\"proiect la plp\",\"has_version\":true, \"version\":2}",
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));

//  try(handler_requests(sqlite3_descriptor, "{\"message_type\":\"get_differences_request\", "
//										   "\"repository_name\":\"proiect la plp\",\"has_version\":true, \"version\":3}",
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));

 JSON_Value *val = json_parse_file("json.json");
////// printf("YEES");
 char ptr[700];
 strcpy(ptr, json_serialize_to_string(val));
//// JSON_Object *jsobject = json_value_get_object(jsvalue);
//// json_object_clear(jsobject);
 try(handler_requests(sqlite3_descriptor, ptr, jsvalue))
  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
// printf("%s", json_serialize_to_string(val));
//  char *pusher="{\"repository_name\":\"bananierul\",\"msg_type\": \"push_request\",\"username\": \"catalin\",\"password\":\"parola1\",\"changelog\": \"initial commit baby\",\"filenames\": [\"cainele.cpp\",\"manelus.c\",\"cici\".rs\"],\"filecontents\": [\"#include<iostream> class caine{}; int main(){ caine c; return MAXIMUS;}\","
//   "\"#include<stdio.h>"
//   " #include<stdlib.h> int main(int argc, char**argv) {return -1;}\",\"fn main(){println!(\"{}\", 4);]}";
//  try(handler_requests(sqlite3_descriptor, pusher,
//					   jsvalue))
//  printf("response: '%s'\n", json_serialize_to_string_pretty(jsvalue));
//
  return 0;
}