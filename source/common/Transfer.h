//
// Created by Marincia Cătălin on 22.12.2020.
//

#ifndef DBINTERACT_SOURCE_COMMON_TRANSFER_H_
#define DBINTERACT_SOURCE_COMMON_TRANSFER_H_

#include "Macros.h"

typedef enum { ENCRYPT, DECRYPT } CryptMode;

void caesar_cipher(char *input, CryptMode mode, u32 key);

i32 write_with_retry(i32 destination_file_descriptor, const void *buffer, u32 expected_no_bytes_written);
i32 read_with_retry(i32 source_file_descriptor, void *buffer, u32 expected_no_bytes_read);

i32 write_with_prefix_secure(i32 destination_file_descriptor, const char *buffer, u32 expected_no_bytes_written, u32 key);
i32 read_with_prefix_secure(i32 source_file_descriptor, char *buffer, u32 key);

i32 write_data_three_pass(i32 destination_file_descriptor, const char *buffer, u32 expected_no_bytes_written, u32 key);
i32 read_data_three_pass(i32 source_file_descriptor, char *buffer, u32 key);
u32 generate_random_key(void);

#endif
