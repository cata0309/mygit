//
// Created by Marincia Cătălin on 22.12.2020.
//

// TPC = Three-Pass protocol Caesar cipher
#include "Transfer.h"
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void caesar_cipher(char *input,
				   const CryptMode mode,
				   const u32 key) {
  i8 direction = ((mode == ENCRYPT) ? 1 : -1);
  for (u64 i = 0; i < strlen(input); ++i) {
	input[i] = (char)(input[i] + direction * key);
  }
}

i32 write_with_retry(i32 destination_file_descriptor,
					 const void *buffer,
					 u32 expected_no_bytes_written) {
  // handle partial writes
  i32 bytesWritten;
  i32 totalBytesWritten = 0;
  const char *ptrBuffer = buffer;
  while (expected_no_bytes_written > 0) {
	do {
	  bytesWritten = write(destination_file_descriptor, ptrBuffer, expected_no_bytes_written);
	  if (bytesWritten != -1) {
		totalBytesWritten += bytesWritten;
	  }
	} while ((bytesWritten < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
	if (bytesWritten < 0)
	  return bytesWritten;
	expected_no_bytes_written -= bytesWritten;
	ptrBuffer += bytesWritten;
  }
  return totalBytesWritten;
}

i32 read_with_retry(i32 source_file_descriptor,
					void *buffer,
					u32 expected_no_bytes_read) {
  // handle partial reads
  char *ptrBuffer = buffer;
  i32 bytesRead;
  i32 totalBytesRead = 0;
  while ((bytesRead = read(source_file_descriptor, ptrBuffer, expected_no_bytes_read)) != 0) {
	if (bytesRead == -1) {
	  return -1;
	}
	totalBytesRead += bytesRead;
	ptrBuffer += bytesRead;
	expected_no_bytes_read -= bytesRead;
  }
  return totalBytesRead;
}

i32 write_with_prefix_secure(i32 destination_file_descriptor,
							 const char *buffer,
							 const u32 expected_no_bytes_written,
							 const u32 key) {
  char *localBuffer = (char *)(malloc(expected_no_bytes_written * sizeof(char)));
  strcpy(localBuffer, buffer);
  caesar_cipher(localBuffer, ENCRYPT, key);
  if (write_with_retry(destination_file_descriptor, &expected_no_bytes_written, sizeof(u32)) != sizeof(u32)) {
	return -1;
  }
  i32 bytesWritten = write_with_retry(destination_file_descriptor, localBuffer, expected_no_bytes_written);
  free(localBuffer);
  return bytesWritten;
}

i32 read_with_prefix_secure(i32 source_file_descriptor,
							char *buffer,
							const u32 key) {
  u32 expected_no_bytes_read;
  CHECKRET(read_with_retry(source_file_descriptor, &expected_no_bytes_read, sizeof(u32)) == sizeof(u32), -1,
		   "Cannot read the size of the string that has to be read")
  i32 bytes_read = read_with_retry(source_file_descriptor, buffer, expected_no_bytes_read);
  if (bytes_read == expected_no_bytes_read) {
	*((char *)(&buffer[bytes_read])) = '\0';
	caesar_cipher(buffer, DECRYPT, key);
	return bytes_read;
  }
  return -1;
}

i32 write_data_three_pass(i32 destination_file_descriptor,
						  const char *buffer,
						  const u32 expected_no_bytes_written,
						  const u32 key) {
  CHECKRET(write_with_prefix_secure(destination_file_descriptor, buffer, expected_no_bytes_written, key), -1,
		   "Three-pass(send): stage 1, cannot encrypt and send the message")
  char *other_encrypted_data = (char *)(malloc(expected_no_bytes_written));
  CHECKRET(read_with_prefix_secure(destination_file_descriptor, other_encrypted_data, key) == expected_no_bytes_written, -1,
		   "Three-pass(send): stage 2, cannot read double encrypted message")
  CHECKRET(write_with_retry(destination_file_descriptor, &expected_no_bytes_written, sizeof(u32)) == sizeof(u32), -1,
		   "Three-pass(send): stage 3, cannot send the size of the encrypted message")
  CHECKRET(
	  write_with_retry(destination_file_descriptor, other_encrypted_data, expected_no_bytes_written)
		  == expected_no_bytes_written, -1,
	  "Three-pass(send): stage 3, cannot send the encrypted message")
  free(other_encrypted_data);
  return 0;
}

i32 read_data_three_pass(i32 source_file_descriptor, char *buffer, u32 key) {
  u32 length;
  CHECKRET(read_with_retry(source_file_descriptor, &length, sizeof(u32)) == sizeof(u32), -1,
		   "Three-pass(receive): stage 1, cannot read the size of the encrypted message")
  CHECKRET(read_with_retry(source_file_descriptor, buffer, length) == length, -1,
		   "Three-pass(receive): stage 1, cannot read the encrypted message")
  buffer[length] = '\0';
  CHECKRET(write_with_prefix_secure(source_file_descriptor, buffer, length, key), -1,
		   "Three-pass(receive): stage 2, cannot write the double encrypted message")
  CHECKRET(read_with_prefix_secure(source_file_descriptor, buffer, key), -1,
		   "Three-pass(receive): stage 3, cannot write the double encrypted message")
  return 0;
}

u32 generate_random_key(void) {
  srand(time(NULL));
  return (rand() % (MAX_KEY + 1 - MIN_KEY)) + MIN_KEY;
}