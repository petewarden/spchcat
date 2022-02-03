#include "flags.h"

#include "acutest.h"

// Include the original source file so we can test static functions.
#include "flags.c"

void test_GetFlagWithName() {
  const char* some_name = "some_value";
  int32_t some_other_name = 10;
  const FlagDefinition test_flags[] = {
    YARGS_STRING("some_name", NULL, &some_name, "Test value."),
    YARGS_INT32("some_other_name", NULL, &some_other_name, "Another test."),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const FlagDefinition* result = GetFlagWithName(test_flags, test_flags_length,
    "some_name");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_name", result->name);
  TEST_STREQ("some_value", *(result->string_value));

  result = GetFlagWithName(test_flags, test_flags_length, "some_other_name");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_other_name", result->name);
  TEST_INTEQ(10, *(result->int32_value));

  result = GetFlagWithName(test_flags, test_flags_length, "nonexistent");
  TEST_CHECK(result == NULL);
}

void test_GetFlagWithShortName() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  int32_t some_other_name = 10;
  const FlagDefinition test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Test value."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_INT32("some_other_name", "b", &some_other_name, "Another test."),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const FlagDefinition* result = GetFlagWithShortName(test_flags, test_flags_length,
    "a");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_name", result->name);
  TEST_STREQ("a", result->short_name);
  TEST_STREQ("some_value", *(result->string_value));

  result = GetFlagWithShortName(test_flags, test_flags_length, "b");
  TEST_ASSERT(result != NULL);
  TEST_STREQ("some_other_name", result->name);
  TEST_STREQ("b", result->short_name);
  TEST_INTEQ(10, *(result->int32_value));

  result = GetFlagWithShortName(test_flags, test_flags_length, "z");
  TEST_CHECK(result == NULL);
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
  const char* some_name = "some_value";
  int32_t some_other_name = 0;
  const FlagDefinition good_flags[] = {
    YARGS_STRING("some_name", NULL, &some_name, "Test value."),
    YARGS_INT32("some_other_name", NULL, &some_other_name, "Another test."),
  };
  bool result = ValidateFlagDefinitions(&good_flags[0],
    sizeof(good_flags) / sizeof(good_flags[0]));
  TEST_CHECK(result);

  const FlagDefinition bad_type_flags[] = {
    {"some_name", NULL, -1, NULL, NULL, NULL, NULL, "Bad type."},
  };
  result = ValidateFlagDefinitions(&bad_type_flags[0],
    sizeof(bad_type_flags) / sizeof(bad_type_flags[0]));
  TEST_CHECK(!result);

  const char* bad_name_string = "Bad name string";
  const FlagDefinition bad_name_flags[] = {
    {NULL, NULL, FT_STRING, NULL, NULL, NULL, &bad_name_string, "Bad name."},
  };
  result = ValidateFlagDefinitions(&bad_name_flags[0],
    sizeof(bad_name_flags) / sizeof(bad_name_flags[0]));
  TEST_CHECK(!result);

  const FlagDefinition bad_ptr_flags[] = {
    {NULL, NULL, FT_STRING, NULL, NULL, NULL, NULL, "Bad pointer."},
  };
  result = ValidateFlagDefinitions(&bad_ptr_flags[0],
    sizeof(bad_ptr_flags) / sizeof(bad_ptr_flags[0]));
  TEST_CHECK(!result);
}

void test_Flags_Init() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  bool some_other_name = false;
  float float_arg = 23.0f;
  FlagDefinition test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Some name."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_BOOL("some_other_name", "b", &some_other_name, "Some other name."),
    YARGS_FLOAT("float_arg", "f", &float_arg, "Float argument"),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const char* argv1[] = {
    "program", "--some_name", "new_value", "unnamed1", "--no_short_name=32",
    "--some_other_name", "--float_arg", "-99.9", "unnamed2",
  };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);

  Flags_Init(test_flags, test_flags_length, argv1, argc1);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(32, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);

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