#include "string_utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"

bool string_starts_with(const char* string, const char* start) {
  return (strncmp(string, start, strlen(start)) == 0);
}

bool string_ends_with(const char* string, const char* ending) {
  const int string_length = strlen(string);
  const int ending_length = strlen(ending);
  if (string_length < ending_length) {
    return false;
  }

  for (int i = 0; i < ending_length; ++i) {
    const int string_index = (string_length - (i + 1));
    const int ending_index = (ending_length - (i + 1));
    const char string_char = string[string_index];
    const char ending_char = ending[ending_index];
    if (string_char != ending_char) {
      return false;
    }
  }

  return true;
}

char* string_duplicate(const char* source) {
  if (source == NULL) {
    return NULL;
  }
  const int length = strlen(source);
  char* result = malloc(length + 1);
  strncpy(result, source, length + 1);
  return result;
}

char* string_alloc_sprintf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  size_t size = vsnprintf(NULL, 0, format, args) + 1;
  va_end(args);
  char* result = malloc(size);
  vsnprintf(result, size, format, args_copy);
  va_end(args_copy);
  return result;
}

// Splits a string into multiple parts, based on the single-character separator.
// The `max_splits` arguments controls the maximum number of parts that will be
// produced, or -1 for no maximum. The caller is responsible for calling free() 
// on the `outputs` array, and for all of the entries in that array.
void string_split(const char* input, char separator, const int max_splits,
  char*** outputs, int* outputs_length) {
  assert(input != NULL);
  const int input_length = strlen(input);
  *outputs = NULL;
  *outputs_length = 0;
  int last_split_index = 0;
  for (int i = 0; i < input_length; ++i) {
    const char current = input[i];
    if ((current == separator) &&
      ((max_splits == -1) || (*outputs_length < (max_splits - 1)))) {
      const int split_length = (i - last_split_index);
      char* split = malloc(split_length + 1);
      for (int j = 0; j < split_length; ++j) {
        split[j] = input[last_split_index + j];
      }
      split[split_length] = 0;
      *outputs = realloc(*outputs, (*outputs_length + 1) * sizeof(char*));
      (*outputs)[*outputs_length] = split;
      *outputs_length += 1;
      last_split_index = i + 1;
    }
  }
  const int split_length = (input_length - last_split_index);
  if (split_length > 0) {
    char* split = malloc(split_length + 1);
    for (int j = 0; j < split_length; ++j) {
      split[j] = input[last_split_index + j];
    }
    split[split_length] = 0;
    *outputs = realloc(*outputs, (*outputs_length + 1) * sizeof(char*));
    (*outputs)[*outputs_length] = split;
    *outputs_length += 1;
  }
}

void string_list_free(char** list, int list_length) {
  for (int i = 0; i < list_length; ++i) {
    free(list[i]);
  }
  free(list);
}

void string_list_add(const char* new, char*** list, int* list_length) {
  *list = realloc(*list, (*list_length + 1) * sizeof(char*));
  (*list)[*list_length] = string_duplicate(new);
  *list_length += 1;
}

char* string_append(const char* a, const char* b) {
  return string_alloc_sprintf("%s%s", a, b);
}

char* string_append_in_place(char* a, const char* b) {
  char* result = string_alloc_sprintf("%s%s", a, b);
  free(a);
  return result;
}

char* string_join(const char** list, int list_length, const char* separator) {
  char* current = string_duplicate("");
  for (int i = 0; i < list_length; ++i) {
    char* next = string_append(current, list[i]);
    free(current);
    current = next;
    if (i < (list_length - 1)) {
      char* next = string_append(current, separator);
      free(current);
      current = next;
    }
  }
  return current;
}

void string_list_filter(const char** in_list, int in_list_length,
  string_list_filter_funcptr should_keep_func, void* cookie, char*** out_list,
  int* out_list_length) {
  *out_list = NULL;
  *out_list_length = 0;
  for (int i = 0; i < in_list_length; ++i) {
    const char* in = in_list[i];
    if (should_keep_func(in, cookie)) {
      *out_list_length += 1;
      *out_list = realloc(*out_list, sizeof(const char*) * (*out_list_length));
      (*out_list)[(*out_list_length) - 1] = string_duplicate(in);
    }
  }
}
