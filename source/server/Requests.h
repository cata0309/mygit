//
// Created by Marincia Cătălin on 19.12.2020.
//

#ifndef DBINTERACT__REQUESTS_H_
#define DBINTERACT__REQUESTS_H_
#include "DatabaseInteraction.h"

u32 get_epoch_time(void);

bool rq_login(sqlite3 *sqlite3_descriptor, JSON_Object *request_object);

bool rq_register(sqlite3 *sqlite3_descriptor, JSON_Object *request_object, JSON_Object *response_object);

bool rq_allow_access_or_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object, JSON_Object *response_object,
							 bool allow_access_mode);

bool rq_block_access_or_edit(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object, JSON_Object *response_object,
							 bool block_access_mode);

bool rq_push(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object, JSON_Object *response_object);

bool rq_checkout_file(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object, JSON_Object *response_object);

bool rq_get_changelog(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object, JSON_Object *response_object);

bool rq_list_of_remote_files(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
							 JSON_Object *response_object);
bool rq_pull_checkout_clone_or_difference(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
										  JSON_Object *response_object, bool difference_mode);

bool rq_switch_repository_access(sqlite3 *sqlite3_descriptor, const char *repository_name, JSON_Object *request_object,
								 JSON_Object *response_object, bool make_public);

bool rq_main_distributor(sqlite3 *sqlite3_descriptor, const char *buffer, JSON_Value *response_value);
#endif