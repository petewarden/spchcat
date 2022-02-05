#include "file_utils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "string_utils.h"
#include "trace.h"

// Helper type for callback functions.
typedef bool (*file_find_func_ptr)(const char*, void*);

static char* file_find_first_with_callback(const char* folder,
  file_find_func_ptr callback, void* cookie) {
  DIR* dir = opendir(folder);
  if (dir == NULL) {
    return NULL;
  }
  char* result = NULL;
  struct dirent* entry = readdir(dir);
  while (entry != NULL)
  {
    const char* filename = entry->d_name;
    if (callback(filename, cookie)) {
      result = string_duplicate(filename);
      break;
    }
    entry = readdir(dir);
  }
  closedir(dir);
  return result;
}

static bool file_has_prefix(const char* filename, void* cookie) {
  const char* prefix = (const char*)(cookie);
  return string_starts_with(filename, prefix);
}

static bool file_has_suffix(const char* filename, void* cookie) {
  const char* suffix = (const char*)(cookie);
  return string_ends_with(filename, suffix);
}

bool file_does_exist(const char* path) {
  struct stat sb;
  return (stat(path, &sb) == 0);
}

off_t file_size(const char* filename) {
  struct stat st;
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}

char* file_find_one_with_prefix(const char* folder,
  const char* prefix) {
  return file_find_first_with_callback(folder, file_has_prefix, (void*)(prefix));
}

char* file_find_one_with_suffix(const char* folder,
  const char* suffix) {
  return file_find_first_with_callback(folder, file_has_suffix,
    (void*)(suffix));
}

char* file_join_paths(const char* a, const char* b) {
  const int a_length = strlen(a);
  const char* format;
  if (a[a_length - 1] == '/') {
    format = "%s%s";
  }
  else {
    format = "%s/%s";
  }
  return string_alloc_sprintf(format, a, b);
}

bool file_read(const char* filename, char** contents, size_t* contents_length) {
  *contents_length = file_size(filename);
  if (*contents_length == -1) {
    *contents = NULL;
    return false;
  }
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    *contents = NULL;
    return false;
  }
  *contents = malloc(*contents_length);
  fread(*contents, 1, *contents_length, file);
  fclose(file);
  return true;
}

bool file_write(const char* filename, const char* contents, size_t contents_length) {
  FILE* file = fopen(filename, "wb");
  if (file == NULL) {
    return false;
  }
  fwrite(contents, 1, contents_length, file);
  fclose(file);
  return true;
}

bool file_list_dir(const char* folder, char*** list, int* list_length) {
  *list = NULL;
  *list_length = 0;
  DIR* dir = opendir(folder);
  if (dir == NULL) {
    return false;
  }
  struct dirent* entry = readdir(dir);
  while (entry != NULL) {
    *list_length += 1;
    *list = realloc(*list, sizeof(char*) * (*list_length));
    (*list)[(*list_length) - 1] = string_duplicate(entry->d_name);
    entry = readdir(dir);
  }
  closedir(dir);
  return true;
}