#include "yargs.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "termcolor-c.h"
#include "string_utils.h"

// Application name from argv[0], used for usage printout.
static const char* app_name = NULL;

// Any arguments that weren't associated with named flags. Typically used
// for file names.
static char** unnamed_args = NULL;
static int unnamed_args_length = 0;

// We have to allocate some memory for string copies, and to keep track of
// them for freeing we use this array.
static void** free_at_end = NULL;
static int free_at_end_length = 0;

// Find a flag definition for this long name, or return NULL.
static const YargsFlag* GetFlagWithName(const YargsFlag* flags,
  int flags_length, const char* name) {
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    if (strcmp(flag->name, name) == 0) {
      return flag;
    }
  }

  return NULL;
}

// Find a flag definition matching a single-character short name, or
// return NULL.
static const YargsFlag* GetFlagWithShortName(const YargsFlag* flags,
  int flags_length, const char* short_name) {
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    if ((flag->short_name != NULL) &&
      (strcmp(flag->short_name, short_name) == 0)) {
      return flag;
    }
  }
  return NULL;
}

// Splits "--foo=bar" into "--foo", "bar", "-xyz" into "-x", "-y", "-z". This
// makes it easier to parse the arguments. Also skips over the first argument,
// since that just contains the application name.
static void NormalizeArgs(char** argv, int argc, char*** norm_argv, int* norm_argc) {
  *norm_argv = NULL;
  *norm_argc = 0;

  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if ((arg[0] == '-') && (arg[1] == '-')) {
      char** opt_parts = NULL;
      int opt_parts_length = 0;
      string_split(arg, '=', 2, &opt_parts, &opt_parts_length);
      for (int j = 0; j < opt_parts_length; ++j) {
        *norm_argc += 1;
        *norm_argv = realloc(*norm_argv, sizeof(const char*) * (*norm_argc));
        (*norm_argv)[(*norm_argc) - 1] = opt_parts[j];
      }
      free(opt_parts);
    }
    else if ((arg[0] == '-') &&
      (strlen(arg) > 1) && // Ignore single dashes.
      ((arg[1] < '0') || (arg[1] > '9'))) { // Ignore negative numbers.

      char** opt_parts = NULL;
      int opt_parts_length = 0;
      string_split(arg, '=', 2, &opt_parts, &opt_parts_length);
      if (opt_parts_length > 1) {
        // If there's an '=', assume there's only a single short name specified
        // and break the arg into the name and the value.
        for (int j = 0; j < opt_parts_length; ++j) {
          *norm_argc += 1;
          *norm_argv = realloc(*norm_argv, sizeof(const char*) * (*norm_argc));
          (*norm_argv)[(*norm_argc) - 1] = opt_parts[j];
        }
        free(opt_parts);
      }
      else {
        string_list_free(opt_parts, opt_parts_length);
        const char* short_opts = &(arg[1]);
        const int short_opts_length = strlen(short_opts);
        for (int j = 0; j < short_opts_length; ++j) {
          char* short_opt = malloc(3);
          short_opt[0] = '-';
          short_opt[1] = short_opts[j];
          short_opt[2] = 0;
          *norm_argc += 1;
          *norm_argv = realloc(*norm_argv, sizeof(const char*) * (*norm_argc));
          (*norm_argv)[(*norm_argc) - 1] = short_opt;
        }
      }
    }
    else {
      int arg_length = strlen(arg);
      char* arg_copy = malloc(arg_length + 1);
      strcpy(arg_copy, arg);
      *norm_argc += 1;
      *norm_argv = realloc(*norm_argv, sizeof(char*) * (*norm_argc));
      (*norm_argv)[(*norm_argc) - 1] = arg_copy;
    }
  }
}

// Give back the memory we allocated to hold the normalized arguments.
static void FreeNormalizedArgs(char** norm_argv, int norm_argc) {
  for (int i = 0; i < norm_argc; ++i) {
    free(norm_argv[i]);
  }
  free(norm_argv);
}

// A friendly wrapper around the raw strtof() numerical string parsing. This
// only returns true if the whole string could be understood as a number.
static bool InterpretValueAsFloat(const char* string, float* output) {
  char* conversion_end = NULL;
  *output = strtof(string, &conversion_end);
  const int converted_length = (conversion_end - string);
  if (converted_length != strlen(string)) {
    return false;
  }
  else {
    return true;
  }
}

// A friendly wrapper around the raw strtol() numerical string parsing. This
// only returns true if the whole string could be understood as a number.
static bool InterpretValueAsInt32(const char* string, int32_t* output) {
  char* conversion_end = NULL;
  long int result = strtol(string, &conversion_end, 10);
  *output = (int32_t)(result);
  const int converted_length = (conversion_end - string);
  if (converted_length != strlen(string)) {
    return false;
  }
  else {
    return true;
  }
}

// Perform checks on the input data supplied by the caller, to ensure there are
// no obvious logical errors.
static bool ValidateYargsFlags(const YargsFlag* flags, int flags_length) {
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    if ((flag->name == NULL)) {
      fprintf(stderr, "Missing name in flag definition #%d.\n", i);
      return false;
    }
    if (strlen(flag->name) < 2) {
      fprintf(stderr, "Name '%s' is too short in flag definition #%d.\n", flag->name, i);
      return false;
    }
    if ((flag->short_name != NULL) && strlen(flag->short_name) > 1) {
      fprintf(stderr, "Short name '%s' should be one character long in flag definition '%s' (#%d).\n",
        flag->short_name, flag->name, i);
      return false;
    }
    for (int j = i + 1; j < flags_length; ++j) {
      const YargsFlag* other_flag = &flags[j];
      if (strcmp(flag->name, other_flag->name) == 0) {
        fprintf(stderr,
          "Name '%s' is repeated in flag definitions #%d and %d.\n", flag->name,
          i, j);
        return false;
      }
      if ((flag->short_name != NULL) && (other_flag->short_name != NULL) &&
        (strcmp(flag->short_name, other_flag->short_name) == 0)) {
        fprintf(stderr,
          "Short name '%s' is repeated in flag definitions #%d and %d.\n",
          flag->short_name, i, j);
        return false;
      }
    }

    switch (flag->type) {
    case FT_BOOL: {
      if (flag->bool_value == NULL) {
        fprintf(stderr, "Missing bool value for flag definition '%s' (#%d).\n",
          flag->name, i);
        return false;
      }
    } break;
    case FT_FLOAT: {
      if (flag->float_value == NULL) {
        fprintf(stderr, "Missing float value for flag definition '%s' (#%d).\n",
          flag->name, i);
        return false;
      }
    } break;
    case FT_INT32: {
      if (flag->int32_value == NULL) {
        fprintf(stderr, "Missing integer value for flag definition '%s' (#%d).\n",
          flag->name, i);
        return false;
      }
    } break;
    case FT_STRING: {
      if (flag->string_value == NULL) {
        fprintf(stderr, "Missing string value for flag definition '%s' (#%d).\n",
          flag->name, i);
        return false;
      }
    } break;
    default: {
      fprintf(stderr, "Bad type %d in flag definition '%s' (#%d).\n",
        flag->type, flag->name, i);
      return false;
    } break;
    }
  }
  return true;
}

static void AddToFreeAtEndList(void* ptr) {
  free_at_end_length += 1;
  free_at_end = realloc(free_at_end, sizeof(void*) * free_at_end_length);
  free_at_end[free_at_end_length - 1] = ptr;
}

// Hopefully-portable way of getting the size of a file (see endless 
// StackOverflow threads for why fseek/ftell isn't guaranteed to work on binary
// files).
static off_t FileSize(const char* filename) {
  struct stat st;
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}

static bool LoadFromFileContents(const YargsFlag* flags, size_t flags_length,
  const char* contents) {

  char** argv = NULL;
  int argc = 0;
  string_split(contents, ' ', -1, &argv, &argc);

  const bool result = yargs_init(flags, flags_length, NULL, argv, argc);

  string_list_free(argv, argc);
  return result;
}

bool yargs_init(const YargsFlag* flags, size_t flags_length,
  const char* app_description, char** argv, int argc) {
  assert(flags != NULL);

  if (!ValidateYargsFlags(flags, flags_length)) {
    return false;
  }

  app_name = argv[0];

  // If we're called multiple times, make sure we don't keep appending to the
  // list of unnamed arguments.
  if (unnamed_args != NULL) {
    free(unnamed_args);
    unnamed_args = NULL;
  }
  unnamed_args_length = 0;

  char** norm_argv;
  int norm_argc;
  NormalizeArgs(argv, argc, &norm_argv, &norm_argc);

  for (int i = 0; i < norm_argc; ++i) {
    const char* arg = norm_argv[i];
    const char* next_value = NULL;
    // Whether we should skip over the next arg because it's the value of
    // the current option, and not an option name.
    bool consume_next_value = true;
    if ((i + 1) < norm_argc) {
      next_value = norm_argv[i + 1];
    }
    if ((arg[0] == '-') &&
      (strlen(arg) > 1) &&  // Skip single dashes.
      ((arg[1] < '0') || (arg[1] > '9'))) { // Skip negative numbers.
      const YargsFlag* flag = NULL;
      if (arg[1] == '-') {
        flag = GetFlagWithName(flags, flags_length, &arg[2]);
      }
      else {
        flag = GetFlagWithShortName(flags, flags_length, &arg[1]);
      }
      if (flag == NULL) {
        fprintf(stderr, "Flag '%s' not recognized.\n", arg);
        yargs_print_usage(flags, flags_length, app_description);
        FreeNormalizedArgs(norm_argv, norm_argc);
        return false;
      }
      switch (flag->type) {
      case FT_BOOL: {
        if (next_value == NULL) {
          *(flag->bool_value) = true;
        }
        else if (next_value[0] == '-') {
          // If the next arg is an option name, don't try to intepret it
          // as a value.
          consume_next_value = false;
          *(flag->bool_value) = true;
        }
        else if (
          (strcmp(next_value, "true") == 0) ||
          (strcmp(next_value, "TRUE") == 0) ||
          (strcmp(next_value, "yes") == 0) ||
          (strcmp(next_value, "YES") == 0) ||
          (strcmp(next_value, "1") == 0)) {
          *(flag->bool_value) = true;
        }
        else if (
          (strcmp(next_value, "false") == 0) ||
          (strcmp(next_value, "FALSE") == 0) ||
          (strcmp(next_value, "no") == 0) ||
          (strcmp(next_value, "NO") == 0) ||
          (strcmp(next_value, "0") == 0)) {
          *(flag->bool_value) = true;
        }
        else {
          fprintf(stderr, "Boolean value for argument '%s' is '%s', but needs to be one of "
            "true, false, yes, no, 1, or 0\n", flag->name, next_value);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }
      } break;

      case FT_FLOAT: {
        if (next_value == NULL) {
          fprintf(stderr, "No value found for argument '%s'\n", flag->name);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }

        if (!InterpretValueAsFloat(next_value, flag->float_value)) {
          fprintf(stderr, "Couldn't interpret '%s' as a floating point number for argument '%s'\n",
            next_value, flag->name);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }
      } break;

      case FT_INT32: {
        if (next_value == NULL) {
          fprintf(stderr, "No value found for argument '%s'\n", flag->name);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }
        if (!InterpretValueAsInt32(next_value, flag->int32_value)) {
          fprintf(stderr, "Couldn't interpret '%s' as an integer for argument '%s'\n",
            next_value, flag->name);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }
      } break;

      case FT_STRING: {
        if (next_value == NULL) {
          fprintf(stderr, "No value found for argument '%s'\n", flag->name);
          FreeNormalizedArgs(norm_argv, norm_argc);
          return false;
        }
        const int next_value_length = strlen(next_value);
        char* next_string = malloc(next_value_length + 1);
        AddToFreeAtEndList(next_string);
        strcpy(next_string, next_value);
        *(flag->string_value) = next_string;
      } break;

      default: {
        fprintf(stderr, "Unknown type %d in definition for flag '%s'\n", flag->type, flag->name);
        FreeNormalizedArgs(norm_argv, norm_argc);
        return false;
      }
      }
    }
    else {
      unnamed_args_length += 1;
      unnamed_args = realloc(unnamed_args, sizeof(const char*) * unnamed_args_length);
      const int arg_length = strlen(arg);
      char* arg_copy = malloc(arg_length + 1);
      AddToFreeAtEndList(arg_copy);
      strcpy(arg_copy, arg);
      unnamed_args[unnamed_args_length - 1] = arg_copy;
      consume_next_value = false;
    }

    // If the following argument in the list was used as a value by this option,
    // skip over it.
    if (consume_next_value) {
      i += 1;
    }
  }

  FreeNormalizedArgs(norm_argv, norm_argc);
  return true;
}

void yargs_free() {
  for (int i = 0; i < free_at_end_length; ++i) {
    free(free_at_end[i]);
  }
  free(free_at_end);
  free_at_end = NULL;
  free_at_end_length = 0;

  free(unnamed_args);
  unnamed_args = NULL;
  unnamed_args_length = 0;
}

void yargs_print_usage(const YargsFlag* flags, int flags_length,
  const char* app_description) {
  text_bold(stderr);
  fprintf(stderr, "Usage");
  reset_colors(stderr);
  text_red(stderr);
  fprintf(stderr, ": %s ", app_name);
  reset_colors(stderr);
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    text_green(stderr);
    fprintf(stderr, "--%s", flag->name);
    reset_colors(stderr);
    if (flag->short_name != NULL) {
      fprintf(stderr, "/");
      text_green(stderr);
      fprintf(stderr, "-%s", flag->short_name);
      reset_colors(stderr);
    }
    fprintf(stderr, " ");
    switch (flag->type) {
    case FT_BOOL: {
      // Do nothing.
    } break;
    case FT_FLOAT: {
      fprintf(stderr, "<float> ");
    } break;
    case FT_INT32: {
      fprintf(stderr, "<integer> ");
    } break;
    case FT_STRING: {
      fprintf(stderr, "<string> ");
    } break;
    default: {
      assert(false);
    } break;
    }
  }
  fprintf(stderr, "\n");
  if (app_description != NULL) {
    text_bold(stderr);
    fprintf(stderr, "%s\n", app_description);
    reset_colors(stderr);
  }
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    text_green(stderr);
    fprintf(stderr, "--%s", flag->name);
    reset_colors(stderr);
    if (flag->short_name != NULL) {
      fprintf(stderr, "/");
      text_green(stderr);
      fprintf(stderr, "-%s", flag->short_name);
      reset_colors(stderr);
    }
    fprintf(stderr, "\t");
    if (flag->description != NULL) {
      text_cyan(stderr);
      fprintf(stderr, "%s\n", flag->description);
      reset_colors(stderr);
    }
  }
}

bool yargs_load_from_file(const YargsFlag* flags, int flags_length,
  const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Flags_LoadFromIniFile(): Couldn't load file '%s'\n",
      filename);
    return false;
  }

  const off_t file_size = FileSize(filename);
  if (file_size <= 0) {
    fprintf(stderr, "Flags_LoadFromIniFile(): File size error loading '%s'\n",
      filename);
    return false;
  }

  char* file_contents = malloc(file_size + 1);
  fread(file_contents, 1, file_size, file);
  file_contents[file_size] = 0;
  const bool result = LoadFromFileContents(flags, flags_length,
    file_contents);
  free(file_contents);

  return result;
}

bool yargs_save_to_file(const YargsFlag* flags, int flags_length,
  const char* filename) {

  FILE* file = fopen(filename, "w");
  if (file == NULL) {
    fprintf(stderr, "yargs_save_to_file: Couldn't write to file '%s'\n",
      filename);
    return false;
  }

  fprintf(file, "%s ", app_name);
  for (int i = 0; i < flags_length; ++i) {
    const YargsFlag* flag = &flags[i];
    fprintf(file, "--%s ", flag->name);
    switch (flag->type) {
    case FT_BOOL: {
      if (*(flag->bool_value)) {
        fprintf(file, "true");
      }
      else {
        fprintf(file, "false");
      }
    } break;
    case FT_FLOAT: {
      fprintf(file, "%f", *(flag->float_value));
    } break;
    case FT_INT32: {
      fprintf(file, "%d", *(flag->int32_value));
    } break;
    case FT_STRING: {
      fprintf(file, "%s", *(flag->string_value));
    } break;
    default: {
      assert(false);
      return false;
    } break;
    }
    if (i < (flags_length - 1)) {
      fprintf(file, " ");
    }
  }

  for (int i = 0; i < unnamed_args_length; ++i) {
    fprintf(file, " %s", unnamed_args[i]);
  }

  fclose(file);
  return true;
}

int yargs_get_unnamed_length() {
  return unnamed_args_length;
}

const char* yargs_get_unnamed(int index) {
  if ((index < 0) || (index >= unnamed_args_length)) {
    return NULL;
  }
  else {
    return unnamed_args[index];
  }
}

const char* yargs_app_name() {
  return app_name;
}
