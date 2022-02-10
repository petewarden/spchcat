#ifndef INCLUDE_YARGS_H
#define INCLUDE_YARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __CPLUSPLUS
extern "C" {
#endif  // __CPLUSPLUS

  typedef enum YargsFlagTypeEnum {
    FT_INT32 = 0,
    FT_FLOAT = 1,
    FT_BOOL = 2,
    FT_STRING = 3,
  } YargsFlagType;

  typedef struct YargsFlagStruct {
    char* name;
    char* short_name;
    YargsFlagType type;
    bool* bool_value;
    float* float_value;
    int32_t* int32_value;
    const char** string_value;
    char* description;
  } YargsFlag;

  // Macros used to simplify adding flag entries.
#define YARGS_BOOL(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_BOOL, VARIABLE_ADDR, NULL, NULL, NULL, DESCRIPTION }

#define YARGS_FLOAT(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_FLOAT, NULL, VARIABLE_ADDR, NULL, NULL, DESCRIPTION }

#define YARGS_INT32(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_INT32, NULL, NULL, VARIABLE_ADDR, NULL, DESCRIPTION }

#define YARGS_STRING(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_STRING, NULL, NULL, NULL, VARIABLE_ADDR, DESCRIPTION }

  bool yargs_init(const YargsFlag* flags, size_t flags_length,
    const char* app_description, char** argv, int argc);
  void yargs_free();

  void yargs_print_usage(const YargsFlag* flags, int flags_length,
    const char* app_description);

  bool yargs_load_from_file(const YargsFlag* flags, int flags_length,
    const char* filename);
  bool yargs_save_to_file(const YargsFlag* flags, int flags_length,
    const char* filename);

  int yargs_get_unnamed_length();
  const char* yargs_get_unnamed(int index);

  const char* yargs_app_name();

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_YARGS_H