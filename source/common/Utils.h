//
// Created by Marincia Cătălin on 22.12.2020.
//

#ifndef DBINTERACT_SOURCE_COMMON_UTILS_H_
#define DBINTERACT_SOURCE_COMMON_UTILS_H_

#include "Macros.h"
#include <stdbool.h>

i32 write_with_retry(i32 destination_file_descriptor,const void *buffer, u32 expected_no_bytes_written);
i32 read_with_retry(i32 source_file_descriptor, void *buffer, u32 expected_no_bytes_read);

bool write_with_prefix(i32 destination_file_descriptor, const char *buffer, u32 expected_no_bytes_written);
bool read_with_prefix(i32 source_file_descriptor, char *buffer);

bool util_is_address_valid(const char *ip_address);
#endif
