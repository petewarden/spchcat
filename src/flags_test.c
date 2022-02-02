#include "flags.h"

#include "acutest.h"

// Include the original source file so we can test static functions.
#include "flags.c"

void test_GetFlagWithName() {
  FlagDefinition test_flags[] = {
    {"some_name", NULL, FT_STRING, 0, 0.0f, false, "some_value"},
    {"some_other_name", NULL, FT_INT32, 10, 0.0f, false, NULL},
  };
  flags = test_flags;
  flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  FlagDefinition* result = GetFlagWithName("some_name");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_name", result->name);
  TEST_STREQ("some_value", result->string_value);

  result = GetFlagWithName("some_other_name");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_other_name", result->name);
  TEST_INTEQ(10, result->int32_value);

  result = GetFlagWithName("nonexistent");
  TEST_CHECK(result == NULL);

  flags = NULL;
}

void test_GetFlagWithShortName() {
  FlagDefinition test_flags[] = {
    {"some_name", "a", FT_STRING, 0, 0.0f, false, "some_value"},
    {"no_short_name", NULL, FT_INT32, 10, 0.0f, false, NULL},
    {"some_other_name", "b", FT_INT32, 10, 0.0f, false, NULL},
  };
  flags = test_flags;
  flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  FlagDefinition* result = GetFlagWithShortName("a");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_name", result->name);
  TEST_STREQ("a", result->short_name);
  TEST_STREQ("some_value", result->string_value);

  result = GetFlagWithShortName("b");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_other_name", result->name);
  TEST_STREQ("b", result->short_name);
  TEST_INTEQ(10, result->int32_value);

  result = GetFlagWithShortName("z");
  TEST_CHECK(result == NULL);

  flags = NULL;
}

void test_Split() {
  char** parts = NULL;
  int parts_length = 0;
  Split("nosepshere", ':', -1, &parts, &parts_length);
  TEST_INTEQ(1, parts_length);
  TEST_STREQ("nosepshere", parts[0]);
  SplitFree(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  Split("seps:r:us", ':', -1, &parts, &parts_length);
  TEST_INTEQ(3, parts_length);
  TEST_STREQ("seps", parts[0]);
  TEST_STREQ("r", parts[1]);
  TEST_STREQ("us", parts[2]);
  SplitFree(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  Split("too-many-seps", '-', 2, &parts, &parts_length);
  TEST_INTEQ(2, parts_length);
  TEST_STREQ("too", parts[0]);
  TEST_STREQ("many-seps", parts[1]);
  SplitFree(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  Split("weird!trailing!sep!", '!', -1, &parts, &parts_length);
  TEST_INTEQ(3, parts_length);
  TEST_STREQ("weird", parts[0]);
  TEST_STREQ("trailing", parts[1]);
  TEST_STREQ("sep", parts[2]);
  SplitFree(parts, parts_length);
}

void test_NormalizeArgs() {
  const char* argv[] = {
    "progname",
    "unnamed",
    "--flag1",
    "value1",
    "--flag2=value2",
    "-s",
    "value3",
    "-xyz",
    "-",
    "-99.9",
    "anotherunnamed"
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);

  char** norm_argv = NULL;
  int norm_argc = 0;
  NormalizeArgs(argv, argc, &norm_argv, &norm_argc);
  TEST_INTEQ(13, norm_argc);
  TEST_STREQ("unnamed", norm_argv[0]);
  TEST_STREQ("--flag1", norm_argv[1]);
  TEST_STREQ("value1", norm_argv[2]);
  TEST_STREQ("--flag2", norm_argv[3]);
  TEST_STREQ("value2", norm_argv[4]);
  TEST_STREQ("-s", norm_argv[5]);
  TEST_STREQ("value3", norm_argv[6]);
  TEST_STREQ("-x", norm_argv[7]);
  TEST_STREQ("-y", norm_argv[8]);
  TEST_STREQ("-z", norm_argv[9]);
  TEST_STREQ("-", norm_argv[10]);
  TEST_STREQ("-99.9", norm_argv[11]);
  TEST_STREQ("anotherunnamed", norm_argv[12]);

  FreeNormalizedArgs(norm_argv, norm_argc);
}

void test_InterpretValueAsFloat() {
  float output = 0.0f;
  bool status = InterpretValueAsFloat("10.0", &output);
  TEST_CHECK(status);
  TEST_FLTEQ(10.0f, output, 0.0001f);

  output = 0.0f;
  status = InterpretValueAsFloat("-33.3333", &output);
  TEST_CHECK(status);
  TEST_FLTEQ(-33.3333f, output, 0.0001f);

  output = 0.0f;
  status = InterpretValueAsFloat("string", &output);
  TEST_CHECK(!status);

  output = 0.0f;
  status = InterpretValueAsFloat("10.0x", &output);
  TEST_CHECK(!status);
}

void test_InterpretValueAsInt32() {
  int32_t output = 0;
  bool status = InterpretValueAsInt32("10", &output);
  TEST_CHECK(status);
  TEST_INTEQ(10, output);

  output = 0;
  status = InterpretValueAsInt32("-33", &output);
  TEST_CHECK(status);
  TEST_INTEQ(-33, output);

  output = 0;
  status = InterpretValueAsInt32("string", &output);
  TEST_CHECK(!status);

  output = 0;
  status = InterpretValueAsInt32("10x", &output);
  TEST_CHECK(!status);

  output = 0;
  status = InterpretValueAsInt32("9999.9", &output);
  TEST_CHECK(!status);
}

void test_ValidateFlagDefinitions() {
  FlagDefinition good_flags[] = {
    {"some_name", NULL, FT_STRING, 0, 0.0f, false, "some_value"},
    {"some_other_name", NULL, FT_INT32, 10, 0.0f, false, NULL},
  };
  bool result = ValidateFlagDefinitions(&good_flags[0],
    sizeof(good_flags) / sizeof(good_flags[0]));
  TEST_CHECK(result);

  FlagDefinition bad_flags[] = {
    {"some_name", NULL, -1, 0, 0.0f, false, "some_value"},
    {"some_other_name", NULL, FT_INT32, 10, 0.0f, false, NULL},
  };
  result = ValidateFlagDefinitions(&bad_flags[0],
    sizeof(bad_flags) / sizeof(bad_flags[0]));
  TEST_CHECK(!result);
}

void test_Flags_Init() {
  FlagDefinition test_flags[] = {
    {"some_name", "a", FT_STRING, 0, 0.0f, false, "some_value"},
    {"no_short_name", NULL, FT_INT32, 10, 0.0f, false, NULL},
    {"some_other_name", "b", FT_BOOL, 10, 0.0f, false, NULL},
    {"float_arg", "f", FT_FLOAT, 0, 23.0f, false, NULL},
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const char* argv1[] = {
    "program", "--some_name", "new_value", "unnamed1", "--no_short_name=32",
    "--some_other_name", "--float_arg", "-99.9", "unnamed2",
  };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);

  Flags_Init(test_flags, test_flags_length, argv1, argc1);

  const char* string_result;
  bool status = Flags_GetString("some_name", &string_result);
  TEST_CHECK(status);
  TEST_STREQ("new_value", string_result);

  int32_t int32_result;
  status = Flags_GetInt32("no_short_name", &int32_result);
  TEST_CHECK(status);
  TEST_INTEQ(32, int32_result);

  bool bool_result;
  status = Flags_GetBool("some_other_name", &bool_result);
  TEST_CHECK(status);
  TEST_CHECK(bool_result);

  float float_result;
  status = Flags_GetFloat("float_arg", &float_result);
  TEST_CHECK(status);
  TEST_FLTEQ(-99.9, float_result, 0.0001f);

  TEST_INTEQ(2, Flags_GetUnnamedLength());
  TEST_STREQ("unnamed1", Flags_GetUnnamed(0));
  TEST_STREQ("unnamed2", Flags_GetUnnamed(1));

  Flags_Free();
}

TEST_LIST = {
  {"GetFlagWithName", test_GetFlagWithName},
  {"GetFlagWithShortName", test_GetFlagWithShortName},
  {"Split", test_Split},
  {"NormalizeArgs", test_NormalizeArgs},
  {"InterpretValueAsFloat", test_InterpretValueAsFloat},
  {"InterpretValueAsInt32", test_InterpretValueAsInt32},
  {"ValidateFlagDefinitions", test_ValidateFlagDefinitions},
  {"Flags_Init", test_Flags_Init},
  {NULL, NULL},
};