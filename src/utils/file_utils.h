#ifndef INCLUDE_UTIL_FILE_UTILS_H
#define INCLUDE_UTIL_FILE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __CPLUSPLUS
extern "C" {
#endif  // __CPLUSPLUS

  bool file_does_exist(const char* filename);
  off_t file_size(const char* filename);

  // No order is guaranteed for these functions, so any matching file may be
  // returned. The caller is responsible for freeing the memory after use.
  char* file_find_one_with_prefix(const char* folder, const char* prefix);
  char* file_find_one_with_suffix(const char* folder, const char* suffix);

  char* file_join_paths(const char* a, const char* b);

  bool file_read(const char* filename, char** contents, size_t* contents_length);
  bool file_write(const char* filename, const char* contents,
    size_t contents_length);

  bool file_list_dir(const char* folder, char*** list, int* list_length);

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_UTIL_FILE_UTILS_H
