#include "yargs.h"

#include "acutest.h"

// Include the original source file so we can test static functions.
#include "yargs.c"

void test_GetFlagWithName() {
  const char* some_name = "some_value";
  int32_t some_other_name = 10;
  const YargsFlag test_flags[] = {
    YARGS_STRING("some_name", NULL, &some_name, "Test value."),
    YARGS_INT32("some_other_name", NULL, &some_other_name, "Another test."),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const YargsFlag* result = GetFlagWithName(test_flags, test_flags_length,
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
  const YargsFlag test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Test value."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_INT32("some_other_name", "b", &some_other_name, "Another test."),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const YargsFlag* result = GetFlagWithShortName(test_flags, test_flags_length,
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

void test_NormalizeArgs() {
  char* argv[] = {
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
    "-f=-99.9",
    "anotherunnamed"
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);

  char** norm_argv = NULL;
  int norm_argc = 0;
  NormalizeArgs(argv, argc, &norm_argv, &norm_argc);
  TEST_INTEQ(15, norm_argc);
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
  TEST_STREQ("-f", norm_argv[12]);
  TEST_STREQ("-99.9", norm_argv[13]);
  TEST_STREQ("anotherunnamed", norm_argv[14]);

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

void test_ValidateYargsFlags() {
  const char* some_name = "some_value";
  int32_t some_other_name = 0;
  const YargsFlag good_flags[] = {
    YARGS_STRING("some_name", NULL, &some_name, "Test value."),
    YARGS_INT32("some_other_name", NULL, &some_other_name, "Another test."),
  };
  bool result = ValidateYargsFlags(&good_flags[0],
    sizeof(good_flags) / sizeof(good_flags[0]));
  TEST_CHECK(result);

  const YargsFlag bad_type_flags[] = {
    {"some_name", NULL, -1, NULL, NULL, NULL, NULL, "Bad type."},
  };
  result = ValidateYargsFlags(&bad_type_flags[0],
    sizeof(bad_type_flags) / sizeof(bad_type_flags[0]));
  TEST_CHECK(!result);

  const char* bad_name_string = "Bad name string";
  const YargsFlag bad_name_flags[] = {
    {NULL, NULL, FT_STRING, NULL, NULL, NULL, &bad_name_string, "Bad name."},
  };
  result = ValidateYargsFlags(&bad_name_flags[0],
    sizeof(bad_name_flags) / sizeof(bad_name_flags[0]));
  TEST_CHECK(!result);

  const YargsFlag bad_ptr_flags[] = {
    {NULL, NULL, FT_STRING, NULL, NULL, NULL, NULL, "Bad pointer."},
  };
  result = ValidateYargsFlags(&bad_ptr_flags[0],
    sizeof(bad_ptr_flags) / sizeof(bad_ptr_flags[0]));
  TEST_CHECK(!result);

  const char* repeated_name_string1 = "Repeated name string1";
  const char* repeated_name_string2 = "Repeated name string2";
  const YargsFlag repeated_name_flags[] = {
    YARGS_STRING("repeated_name", NULL, &repeated_name_string1, "Test value."),
    YARGS_STRING("repeated_name", NULL, &repeated_name_string2, "Test value."),
  };
  result = ValidateYargsFlags(&repeated_name_flags[0],
    sizeof(repeated_name_flags) / sizeof(repeated_name_flags[0]));
  TEST_CHECK(!result);

  const YargsFlag repeated_short_name_flags[] = {
    YARGS_STRING("some_name", "a", &repeated_name_string1, "Test value."),
    YARGS_STRING("some_other_name", "a", &repeated_name_string2, "Test value."),
  };
  result = ValidateYargsFlags(&repeated_short_name_flags[0],
    sizeof(repeated_short_name_flags) / sizeof(repeated_short_name_flags[0]));
  TEST_CHECK(!result);
}

void test_yargs_init() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  bool some_other_name = false;
  float float_arg = 23.0f;
  YargsFlag test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Some name."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_BOOL("some_other_name", "b", &some_other_name, "Some other name."),
    YARGS_FLOAT("float_arg", "f", &float_arg, "Float argument"),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  char* argv1[] = {
    "program", "--some_name", "new_value", "unnamed1", "--no_short_name=32",
    "--some_other_name", "--float_arg", "-99.9", "unnamed2",
  };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);
  bool status = yargs_init(test_flags, test_flags_length, NULL, argv1, argc1);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(32, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  char* argv_no_args[] = {
    "program",
  };
  const int argc_no_args = sizeof(argv_no_args) / sizeof(argv_no_args[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_no_args,
    argc_no_args);
  TEST_CHECK(status);
  TEST_STREQ("some_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(!some_other_name);
  TEST_FLTEQ(23.0f, float_arg, 0.0001f);
  TEST_INTEQ(0, yargs_get_unnamed_length());
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  char* argv_short_names[] = {
    "program", "-a", "new_value", "unnamed1", "-b", "-f=-99.9", "unnamed2",
  };
  const int argc_short_names =
    sizeof(argv_short_names) / sizeof(argv_short_names[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_short_names,
    argc_short_names);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();

  char* argv_bad_short[] = {
    "program", "-ab=value",
  };
  const int argc_bad_short = sizeof(argv_bad_short) / sizeof(argv_bad_short[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_bad_short,
    argc_bad_short);
  TEST_CHECK(!status);
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  char* argv_unnamed_only[] = {
    "program", "unnamed1", "unnamed2",
  };
  const int argc_unnamed_only =
    sizeof(argv_unnamed_only) / sizeof(argv_unnamed_only[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_unnamed_only,
    argc_unnamed_only);
  TEST_CHECK(status);
  TEST_STREQ("some_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(!some_other_name);
  TEST_FLTEQ(23.0f, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();

  char* argv_no_such[] = {
    "program", "--no_such",
  };
  const int argc_no_such = sizeof(argv_no_such) / sizeof(argv_no_such[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_no_such,
    argc_no_such);
  TEST_CHECK(!status);
  yargs_free();

  char* argv_no_such_short[] = {
    "program", "-x",
  };
  const int argc_no_such_short =
    sizeof(argv_no_such_short) / sizeof(argv_no_such_short[0]);
  status = yargs_init(test_flags, test_flags_length, NULL, argv_no_such_short,
    argc_no_such_short);
  TEST_CHECK(!status);
  yargs_free();

  const YargsFlag empty_flags[] = {};
  const int empty_flags_length = 0;

  status = yargs_init(empty_flags, empty_flags_length, NULL, argv_no_args,
    argc_no_args);
  TEST_CHECK(status);
  yargs_free();

  status = yargs_init(empty_flags, empty_flags_length, NULL, argv_unnamed_only,
    argc_unnamed_only);
  TEST_CHECK(status);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();
}

void test_LoadFileFromContents() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  bool some_other_name = false;
  float float_arg = 23.0f;
  YargsFlag test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Some name."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_BOOL("some_other_name", "b", &some_other_name, "Some other name."),
    YARGS_FLOAT("float_arg", "f", &float_arg, "Float argument"),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const char* contents1 =
    "program --some_name new_value --no_short_name=32 --some_other_name "
    "--float_arg -99.9";
  bool status = LoadFromFileContents(test_flags, test_flags_length,
    contents1);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(32, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(0, yargs_get_unnamed_length());
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  const char* content_no_args = "program";
  status = LoadFromFileContents(test_flags, test_flags_length, content_no_args);
  TEST_CHECK(status);
  TEST_STREQ("some_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(!some_other_name);
  TEST_FLTEQ(23.0f, float_arg, 0.0001f);
  TEST_INTEQ(0, yargs_get_unnamed_length());
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  const char* content_short_names =
    "program -a new_value unnamed1 -b -f=-99.9 unnamed2";
  status = LoadFromFileContents(test_flags, test_flags_length,
    content_short_names);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();

  const char* content_bad_short = "program -ab=value";
  status = LoadFromFileContents(test_flags, test_flags_length,
    content_bad_short);
  TEST_CHECK(!status);
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;
  const char* content_unnamed_only = "program unnamed1 unnamed2";
  status = LoadFromFileContents(test_flags, test_flags_length,
    content_unnamed_only);
  TEST_CHECK(status);
  TEST_STREQ("some_value", some_name);
  TEST_INTEQ(10, no_short_name);
  TEST_CHECK(!some_other_name);
  TEST_FLTEQ(23.0f, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();

  const char* contents_no_such = "program --no_such";
  status = LoadFromFileContents(test_flags, test_flags_length,
    contents_no_such);
  TEST_CHECK(!status);
  yargs_free();

  const char* contents_no_such_short = "program -x";
  status = LoadFromFileContents(test_flags, test_flags_length,
    contents_no_such_short);
  TEST_CHECK(!status);
  yargs_free();

  const YargsFlag empty_flags[] = {};
  const int empty_flags_length = 0;

  status = LoadFromFileContents(empty_flags, empty_flags_length,
    content_no_args);
  TEST_CHECK(status);
  yargs_free();

  status = LoadFromFileContents(empty_flags, empty_flags_length,
    content_unnamed_only);
  TEST_CHECK(status);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();
}

void test_yargs_load_from_file() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  bool some_other_name = false;
  float float_arg = 23.0f;
  YargsFlag test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Some name."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_BOOL("some_other_name", "b", &some_other_name, "Some other name."),
    YARGS_FLOAT("float_arg", "f", &float_arg, "Float argument"),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  const char* contents1 =
    "program --some_name new_value --no_short_name=32 --some_other_name "
    "--float_arg -99.9";
  const char* test_filename1 = "/tmp/yargs_test_file1.txt";
  FILE* test_file1 = fopen(test_filename1, "w");
  fprintf(test_file1, "%s", contents1);
  fclose(test_file1);
  bool status = yargs_load_from_file(test_flags, test_flags_length,
    test_filename1);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(32, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(0, yargs_get_unnamed_length());
  yargs_free();
}

void test_yargs_save_to_file() {
  const char* some_name = "some_value";
  int32_t no_short_name = 10;
  bool some_other_name = false;
  float float_arg = 23.0f;
  YargsFlag test_flags[] = {
    YARGS_STRING("some_name", "a", &some_name, "Some name."),
    YARGS_INT32("no_short_name", NULL, &no_short_name, "No short name."),
    YARGS_BOOL("some_other_name", "b", &some_other_name, "Some other name."),
    YARGS_FLOAT("float_arg", "f", &float_arg, "Float argument"),
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);

  char* argv1[] = {
    "program", "--some_name", "new_value", "unnamed1", "--no_short_name=32",
    "--some_other_name", "--float_arg", "-99.9", "unnamed2",
  };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);
  bool status = yargs_init(test_flags, test_flags_length, NULL, argv1, argc1);
  TEST_CHECK(status);
  const char* test_filename1 = "/tmp/yargs_test_file1.txt";
  yargs_save_to_file(test_flags, test_flags_length, test_filename1);
  yargs_free();

  some_name = "some_value";
  no_short_name = 10;
  some_other_name = false;
  float_arg = 23.0f;

  status = yargs_load_from_file(test_flags, test_flags_length,
    test_filename1);
  TEST_CHECK(status);
  TEST_STREQ("new_value", some_name);
  TEST_INTEQ(32, no_short_name);
  TEST_CHECK(some_other_name);
  TEST_FLTEQ(-99.9, float_arg, 0.0001f);
  TEST_INTEQ(2, yargs_get_unnamed_length());
  TEST_STREQ("unnamed1", yargs_get_unnamed(0));
  TEST_STREQ("unnamed2", yargs_get_unnamed(1));
  yargs_free();
}

static void test_yargs_app_name() {
  YargsFlag test_flags[] = {
  };
  const int test_flags_length = sizeof(test_flags) / sizeof(test_flags[0]);
  char* argv[] = { "program" };
  const int argc = sizeof(argv) / sizeof(argv[0]);
  bool status = yargs_init(test_flags, test_flags_length, NULL, argv, argc);
  TEST_CHECK(status);
  TEST_STREQ("program", yargs_app_name());
}

TEST_LIST = {
  {"GetFlagWithName", test_GetFlagWithName},
  {"GetFlagWithShortName", test_GetFlagWithShortName},
  {"NormalizeArgs", test_NormalizeArgs},
  {"InterpretValueAsFloat", test_InterpretValueAsFloat},
  {"InterpretValueAsInt32", test_InterpretValueAsInt32},
  {"ValidateYargsFlags", test_ValidateYargsFlags},
  {"yargs_init", test_yargs_init},
  {"LoadFileFromContents", test_LoadFileFromContents},
  {"yargs_load_from_file", test_yargs_load_from_file},
  {"yargs_save_to_file", test_yargs_save_to_file},
  {"yargs_app_name", test_yargs_app_name},
  {NULL, NULL},
};