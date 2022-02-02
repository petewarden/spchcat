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
    int32_t int32_value;
    float float_value;
    bool bool_value;
    const char* string_value;
  } FlagDefinition;

  bool Flags_Init(const FlagDefinition* definitions, size_t definitions_length,
    const char** argv, int argc);
  void Flags_Free();

  void Flags_PrintUsage();

  bool Flags_LoadFromIniFile(const char* filename);
  bool Flags_SaveToIniFile(const char* filename);

  bool Flags_GetInt32(const char* name, int32_t* output);
  bool Flags_GetFloat(const char* name, float* output);
  bool Flags_GetBool(const char* name, bool* output);
  bool Flags_GetString(const char* name, const char** output);

  int Flags_GetUnnamedLength();
  const char* Flags_GetUnnamed(int index);

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_FLAGS_H