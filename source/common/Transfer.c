//
// Created by Marincia Cătălin on 22.12.2020.
//

#include "Transfer.h"
#include <errno.h>
#include <unistd.h>

i32 write_with_retry(i32 destination_file_descriptor, const void *buffer, u32 expected_no_bytes_written) {
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

i32 read_with_retry(i32 source_file_descriptor, void *buffer, u32 expected_no_bytes_read) {
  // handle partial reads
  char *ptr_buffer = buffer;
  i32 bytes_read;
  i32 total_bytes_read = 0;
  while ((bytes_read = read(source_file_descriptor, ptr_buffer, expected_no_bytes_read)) != 0) {
	if (bytes_read == -1) {
	  return -1;
	}
	total_bytes_read += bytes_read;
	ptr_buffer += bytes_read;
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
