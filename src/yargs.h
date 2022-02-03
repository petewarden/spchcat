#ifndef INCLUDE_FLAGS_H
#define INCLUDE_FLAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __CPLUSPLUS
extern "C" {
#endif  // __CPLUSPLUS

  typedef enum FlagTypeEnum {
    FT_INT32 = 0,
    FT_FLOAT = 1,
    FT_BOOL = 2,
    FT_STRING = 3,
  } FlagType;

  typedef struct FlagDefinitionStruct {
    const char* name;
    const char* short_name;
    FlagType type;
    bool* bool_value;
    float* float_value;
    int32_t* int32_value;
    const char** string_value;
    const char* description;
  } FlagDefinition;

  // Macros used to simplify adding flag entries.
#define YARGS_BOOL(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_BOOL, VARIABLE_ADDR, NULL, NULL, NULL, DESCRIPTION }

#define YARGS_FLOAT(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_FLOAT, NULL, VARIABLE_ADDR, NULL, NULL, DESCRIPTION }

#define YARGS_INT32(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_INT32, NULL, NULL, VARIABLE_ADDR, NULL, DESCRIPTION }

#define YARGS_STRING(NAME, SHORT_NAME, VARIABLE_ADDR, DESCRIPTION) {\
  NAME, SHORT_NAME, FT_STRING, NULL, NULL, NULL, VARIABLE_ADDR, DESCRIPTION }

  bool Flags_Init(const FlagDefinition* definitions, size_t definitions_length,
    const char** argv, int argc);
  void Flags_Free();

  void Flags_PrintUsage();

  bool Flags_LoadFromIniFile(const FlagDefinition* flags, int flags_length, const char* filename);
  bool Flags_SaveToIniFile(const FlagDefinition* flags, int flags_length, const char* filename);

  int Flags_GetUnnamedLength();
  const char* Flags_GetUnnamed(int index);

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_FLAGS_H