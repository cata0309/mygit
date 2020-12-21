#ifndef DBINTERACT__REQUESTHANDLERS_H_
#define DBINTERACT__REQUESTHANDLERS_H_
#include "DbOperations.h"

u32 get_current_time(void);
bool embedded_login_request(sqlite3 *sqlite3_descriptor,
							JSON_Object *request_root_object);

bool handler_add_user_request(sqlite3 *sqlite3_descriptor,
							  JSON_Object *request_root_object,
							  JSON_Object *response_root_object);

// <----restricted begin
bool handler_add_permission_or_access_request(sqlite3 *sqlite3_descriptor,
											  const char *repository_name,
											  JSON_Object *request_root_object,
											  JSON_Object *response_root_object,
											  bool add_access_mode);

bool handler_del_permission_or_access_request(sqlite3 *sqlite3_descriptor,
											  const char *repository_name,
											  JSON_Object *request_root_object,
											  JSON_Object *response_root_object,
											  bool delete_access_mode);

bool handler_push_request(sqlite3 *sqlite3_descriptor,
						  const char *repository_name,
						  JSON_Object *request_root_object,
						  JSON_Object *response_root_object);

// <----restricted end
bool handler_checkout_file_request(sqlite3 *sqlite3_descriptor,
								   const char *repository_name,
								   JSON_Object *request_root_object,
								   JSON_Object *response_root_object);

bool handler_get_changelog_request(sqlite3 *sqlite3_descriptor,
								   const char *repository_name,
								   JSON_Object *request_root_object,
								   JSON_Object *response_root_object);

bool handler_ls_remote_files_request(sqlite3 *sqlite3_descriptor,
									 const char *repository_name,
									 JSON_Object *request_root_object,
									 JSON_Object *response_root_object);
// checkout+pull+clone
bool handler_checkout_differences_request(sqlite3 *sqlite3_descriptor,
										  const char *repository_name,
										  JSON_Object *request_root_object,
										  JSON_Object *response_root_object,
										  bool is_difference);

bool handler_make_repo_switch_access_request(sqlite3 *sqlite3_descriptor,
											 const char *repository_name,
											 JSON_Object *request_root_object,
											 JSON_Object *response_root_object,
											 bool make_public);


bool handler_requests(sqlite3 *sqlite3_descriptor, const char *buffer, JSON_Value *response_root_value);
#endif