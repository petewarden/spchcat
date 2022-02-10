#include "acutest.h"

#include "string_utils.c"

void test_string_starts_with() {

  const char* string = "foo.txt";
  const char* start = "foo";
  TEST_CHECK(string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);

  string = "foo.txt";
  start = "bar";
  TEST_CHECK(!string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);

  string = "foo.txt";
  start = "toolongtobeastarts";
  TEST_CHECK(!string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);

  string = "foo.txt";
  start = "";
  TEST_CHECK(string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);

  string = "";
  start = "";
  TEST_CHECK(string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);

  string = "";
  start = "foo";
  TEST_CHECK(!string_starts_with(string, start));
  TEST_MSG("%s, %s", string, start);
}

void test_string_ends_with() {

  const char* string = "foo.txt";
  const char* ending = ".txt";
  TEST_CHECK(string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "short";
  ending = "longer";
  TEST_CHECK(!string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "something";
  ending = "";
  TEST_CHECK(string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "";
  ending = "something";
  TEST_CHECK(!string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "foo.txt.old";
  ending = ".txt";
  TEST_CHECK(!string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "";
  ending = "";
  TEST_CHECK(string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);

  string = "afile.txt";
  ending = "rfile.txt";
  TEST_CHECK(!string_ends_with(string, ending));
  TEST_MSG("%s, %s", string, ending);
}

void test_string_duplicate() {
  const char* original = "original";
  char* copy = string_duplicate(original);
  TEST_STREQ(original, copy);
  free(copy);

  original = "";
  copy = string_duplicate(original);
  TEST_STREQ(original, copy);
  free(copy);

  original = NULL;
  copy = string_duplicate(original);
  TEST_CHECK(copy == NULL);
  TEST_MSG("%p", copy);
  free(copy);
}

void test_string_alloc_sprintf() {

  char* result = string_alloc_sprintf("%s/%s", "foo", "bar");
  TEST_STREQ("foo/bar", result);
  free(result);

  result = string_alloc_sprintf("%s/%d", "foo", 10);
  TEST_STREQ("foo/10", result);
  free(result);

  result = string_alloc_sprintf("%s/0x%08x", "foo", 10);
  TEST_STREQ("foo/0x0000000a", result);
  free(result);
}

void test_string_split() {
  char** parts = NULL;
  int parts_length = 0;
  string_split("nosepshere", ':', -1, &parts, &parts_length);
  TEST_INTEQ(1, parts_length);
  TEST_STREQ("nosepshere", parts[0]);
  string_list_free(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  string_split("seps:r:us", ':', -1, &parts, &parts_length);
  TEST_INTEQ(3, parts_length);
  TEST_STREQ("seps", parts[0]);
  TEST_STREQ("r", parts[1]);
  TEST_STREQ("us", parts[2]);
  string_list_free(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  string_split("too-many-seps", '-', 2, &parts, &parts_length);
  TEST_INTEQ(2, parts_length);
  TEST_STREQ("too", parts[0]);
  TEST_STREQ("many-seps", parts[1]);
  string_list_free(parts, parts_length);

  parts = NULL;
  parts_length = 0;
  string_split("weird!trailing!sep!", '!', -1, &parts, &parts_length);
  TEST_INTEQ(3, parts_length);
  TEST_STREQ("weird", parts[0]);
  TEST_STREQ("trailing", parts[1]);
  TEST_STREQ("sep", parts[2]);
  string_list_free(parts, parts_length);
}

void test_string_append() {
  char* result = string_append("a", "b");
  TEST_STREQ("ab", result);
  free(result);

  result = string_append("", "");
  TEST_STREQ("", result);
  free(result);

  result = string_append("something ", "else");
  TEST_STREQ("something else", result);
  free(result);
}

void test_string_append_in_place() {
  char* original = string_duplicate("original");
  char* result = string_append_in_place(original, "b");
  TEST_STREQ("originalb", result);
  free(result);
}

void test_string_join() {
  const char* list1[] = { "a", "b", "c" };
  const int list1_length = sizeof(list1) / sizeof(list1[0]);

  char* result = string_join(list1, list1_length, ", ");
  TEST_STREQ(result, "a, b, c");
  free(result);

  result = string_join(list1, list1_length, ":");
  TEST_STREQ(result, "a:b:c");
  free(result);

  result = string_join(list1, list1_length, "");
  TEST_STREQ(result, "abc");
  free(result);

  const char* list2[] = { "foo", "", "bar", "baz" };
  const int list2_length = sizeof(list2) / sizeof(list2[0]);

  result = string_join(list2, list2_length, "a");
  TEST_STREQ(result, "fooaabarabaz");
  free(result);

  result = string_join(list2, list2_length, ", ");
  TEST_STREQ(result, "foo, , bar, baz");
  free(result);
}

bool test_string_list_filter_func(const char* a, void* cookie) {
  const char* search = cookie;
  return (strstr(a, search) == NULL);
}

void test_string_list_filter() {
  const char* list[] = {
    "foo",
    "bar",
    "baz",
    "fish",
  };
  const int list_length = sizeof(list) / sizeof(list[0]);

  char** results;
  int results_length;
  string_list_filter(list, list_length, test_string_list_filter_func, "a",
    &results, &results_length);
  TEST_INTEQ(2, results_length);
  TEST_STREQ("foo", results[0]);
  TEST_STREQ("fish", results[1]);
  string_list_free(results, results_length);

  string_list_filter(list, list_length, test_string_list_filter_func, "fo",
    &results, &results_length);
  TEST_INTEQ(3, results_length);
  TEST_STREQ("bar", results[0]);
  TEST_STREQ("baz", results[1]);
  TEST_STREQ("fish", results[2]);
  string_list_free(results, results_length);
}

void test_string_list_add() {
  char** list = NULL;
  int list_length = 0;
  string_list_add("foo", &list, &list_length);
  TEST_INTEQ(1, list_length);
  TEST_STREQ("foo", list[0]);

  string_list_add("bar", &list, &list_length);
  string_list_add("baz", &list, &list_length);
  TEST_INTEQ(3, list_length);
  TEST_STREQ("foo", list[0]);
  TEST_STREQ("bar", list[1]);
  TEST_STREQ("baz", list[2]);

  string_list_free(list, list_length);
}

TEST_LIST = {
  {"string_starts_with", test_string_starts_with},
  {"string_ends_with", test_string_ends_with},
  {"string_duplicate", test_string_duplicate},
  {"string_alloc_sprintf", test_string_alloc_sprintf},
  {"string_split", test_string_split},
  {"string_append", test_string_append},
  {"string_append_in_place", test_string_append_in_place},
  {"string_join", test_string_join},
  {"string_list_filter", test_string_list_filter},
  {"string_list_add", test_string_list_add},
  {NULL, NULL},
};