//
// Created by Marincia Cătălin on 22.12.2020.
//

#include "Utils.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

i32 write_with_retry(i32 destination_file_descriptor, const void *buffer, u32 expected_no_bytes_written) {
  // handle partial writes
  i32 bytes_written, total_bytes_written = 0;
  const char *pointer_buffer = buffer;
  while (expected_no_bytes_written > 0) {
	do {
	  bytes_written = write(destination_file_descriptor, pointer_buffer, expected_no_bytes_written);
	  if (bytes_written != -1) {
		total_bytes_written += bytes_written;
	  }
	} while ((bytes_written < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
	if (bytes_written < 0)
	  return bytes_written;
	expected_no_bytes_written -= bytes_written;
	pointer_buffer += bytes_written;
  }
  return total_bytes_written;
}

i32 read_with_retry(i32 source_file_descriptor, void *buffer, u32 expected_no_bytes_read) {
  // handle partial reads
  char *pointer_buffer = buffer;
  i32 bytes_read, total_bytes_read = 0;
  while ((bytes_read = read(source_file_descriptor, pointer_buffer, expected_no_bytes_read)) != 0) {
	if (bytes_read == -1) {
	  return -1;
	}
	total_bytes_read += bytes_read;
	pointer_buffer += bytes_read;
	expected_no_bytes_read -= bytes_read;
  }
  return total_bytes_read;
}

bool write_with_prefix(i32 destination_file_descriptor, const char *buffer, u32 expected_no_bytes_written) {
  CHECKRET (write_with_retry(destination_file_descriptor, &expected_no_bytes_written, sizeof(u32)) == sizeof(u32), false,
			"Cannot write the size of the string that has to be written")
  CHECKRET(write_with_retry(destination_file_descriptor, buffer, expected_no_bytes_written) == expected_no_bytes_written,
		   false, "Could not write the message")
  return true;
}

bool read_with_prefix(i32 source_file_descriptor, char *buffer) {
  u32 expected_no_bytes_read;
  CHECKRET(read_with_retry(source_file_descriptor, &expected_no_bytes_read, sizeof(u32)) == sizeof(u32), false,
		   "Cannot read the size of the string that has to be read")
  CHECKRET(read_with_retry(source_file_descriptor, buffer, expected_no_bytes_read) == expected_no_bytes_read, false,
		   "Could not read the message")
  buffer[expected_no_bytes_read] = '\0';
  return true;
}

bool util_is_address_valid(const char *ip_address) {
  if (strlen(ip_address) > 15 || strlen(ip_address) < 7)
	return false;
  char section[4];
  if (ip_address == NULL) {
	return false;
  }
  i32 index = 0;
  u8 section_index = 0;
  u16 number;
  u8 dots = 0;
  char *conversion_ptr;
  bool accepting_digits = true;
  while (index < strlen(ip_address)) {
	if (accepting_digits == true) {
	  if (!(ip_address[index] >= '0' && ip_address[index] <= '9')) { return false; }
	  section[section_index] = ip_address[index];
	  section_index += 1;
	  if (section_index == 3 || (index + 1 < strlen(ip_address) && ip_address[index + 1] == '.')) {
		section[section_index] = '\0';
		number = (u16)strtol(section, &conversion_ptr, 10);
		if (!(number >= 0 && number <= 255)) {
		  return false;
		}
		section_index = 0;
		accepting_digits = false;
	  }
	} else if (ip_address[index] != '.') {
	  return false;
	} else {
	  accepting_digits = true;
	  dots += 1;
	}
	index += 1;
  }
  return dots == 3;
}