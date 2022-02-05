#include "acutest.h"

#include "file_utils.c"

#include <stdio.h>
#include <stdlib.h>

#include "string_utils.h"

void test_file_does_exist() {
  const char* test_filename = "/tmp/file_does_exist_test.txt";
  FILE* test_file = fopen(test_filename, "w");
  fprintf(test_file, "Test contents.");
  fclose(test_file);

  TEST_CHECK(file_does_exist(test_filename));
  TEST_MSG("%s", test_filename);

  const char* nonexistent_filename = "/some/very/unlikely/path/foo.bar";
  TEST_CHECK(!file_does_exist(nonexistent_filename));
}

void test_file_size() {
  const char* test_filename = "/tmp/file_size_test.txt";
  FILE* test_file = fopen(test_filename, "w");
  fprintf(test_file, "Test contents.");
  fclose(test_file);

  off_t size = file_size(test_filename);
  TEST_SIZEQ(14, size);
  TEST_MSG("%s", test_filename);

  const char* nonexistent_filename = "/some/very/unlikely/path/foo.bar";
  size = file_size(nonexistent_filename);
  TEST_SIZEQ(-1, size);
  TEST_MSG("%s", test_filename);
}

void test_file_find_one_with_prefix() {
  char* tmp_dirname = "/tmp/tmpdir_test_file_find_one_with_prefix";
  mkdir(tmp_dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  char* filename = file_join_paths(tmp_dirname, "afile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "anotherfile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "randomfile");
  file_write(filename, "a", 1);
  free(filename);

  char* found = file_find_one_with_prefix(tmp_dirname, "a");
  TEST_CHECK(found != NULL);
  // No order is guaranteed, so just make sure it starts with the prefix.
  TEST_CHECK(found[0] == 'a');
  free(found);

  found = file_find_one_with_prefix(tmp_dirname, "another");
  TEST_CHECK(found != NULL);
  TEST_STREQ("anotherfile.txt", found);
  free(found);

  found = file_find_one_with_prefix(tmp_dirname, "nosuch");
  TEST_CHECK(found == NULL);
  free(found);

  rmdir(tmp_dirname);
}

void test_file_find_one_with_suffix() {
  char* tmp_dirname = "/tmp/tmpdir_test_file_find_one_with_suffix";
  mkdir(tmp_dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  char* filename = file_join_paths(tmp_dirname, "afile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "anotherfile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "randomfile");
  file_write(filename, "a", 1);
  free(filename);

  char* found = file_find_one_with_suffix(tmp_dirname, ".txt");
  TEST_CHECK(found != NULL);
  // No order is guaranteed, so just make sure it starts with the prefix.
  TEST_CHECK(string_ends_with(found, ".txt"));
  TEST_MSG("%s", found);
  free(found);

  found = file_find_one_with_suffix(tmp_dirname, "rfile.txt");
  TEST_CHECK(found != NULL);
  TEST_STREQ("anotherfile.txt", found);
  free(found);

  found = file_find_one_with_suffix(tmp_dirname, "nosuch");
  TEST_CHECK(found == NULL);
  free(found);

  rmdir(tmp_dirname);
}

void test_file_join_paths() {
  const char* a = "/some/path";
  const char* b = "another/path";
  char* result = file_join_paths(a, b);
  TEST_STREQ("/some/path/another/path", result);
  free(result);

  a = "/some/path/";
  b = "another/path";
  result = file_join_paths(a, b);
  TEST_STREQ("/some/path/another/path", result);
  free(result);

  a = "path";
  b = "file.txt";
  result = file_join_paths(a, b);
  TEST_STREQ("path/file.txt", result);
  free(result);
}

void test_file_read() {
  const char* test_filename = "/tmp/file_read_test.txt";
  FILE* test_file = fopen(test_filename, "w");
  fprintf(test_file, "Test contents.%c", 0);
  fclose(test_file);

  char* contents;
  size_t contents_length;
  bool status = file_read(test_filename, &contents, &contents_length);
  TEST_CHECK(status);
  TEST_SIZEQ(15, contents_length);
  TEST_STREQ("Test contents.", contents);
  free(contents);
  contents = NULL;
  contents_length = 0;

  const char* nonexistent_filename = "/some/very/unlikely/path/foo.bar";
  status = file_read(nonexistent_filename, &contents, &contents_length);
  TEST_CHECK(!status);
}

void test_file_write() {
  const char* test_filename = "/tmp/file_read_test.txt";
  const char* contents = "Some test content.";
  const int contents_length = strlen(contents) + 1;
  const bool status = file_write(test_filename, contents, contents_length);
  TEST_CHECK(status);
  TEST_MSG("%s", test_filename);

  FILE* file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  TEST_MSG("%s", test_filename);
  char* read_contents = malloc(contents_length);
  fread(read_contents, 1, contents_length, file);
  fclose(file);
  TEST_STREQ(contents, read_contents);
  free(read_contents);
}

void test_file_list_dir() {
  const char* tmp_dirname = "/tmp/tmpdir_test_file_list_dir";
  mkdir(tmp_dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  char* filename = file_join_paths(tmp_dirname, "afile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "anotherfile.txt");
  file_write(filename, "a", 1);
  free(filename);

  filename = file_join_paths(tmp_dirname, "randomfile");
  file_write(filename, "a", 1);
  free(filename);

  const char* tmp_subdirname = "/tmp/tmpdir_test_file_list_dir/somedir";
  mkdir(tmp_subdirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  char** list;
  int list_length;
  bool status = file_list_dir(tmp_dirname, &list, &list_length);
  TEST_CHECK(status);
  TEST_INTEQ(6, list_length);
  char* list_joined = string_join((const char**)(list), list_length, ", ");

  // Order can be arbitrary, so look for matches anywhere.
  const char* expected_list[] = {
    ".",
    "..",
    "afile.txt",
    "anotherfile.txt",
    "randomfile",
    "somedir",
  };
  const int expected_list_length =
    sizeof(expected_list) / sizeof(expected_list[0]);
  for (int i = 0; i < expected_list_length; ++i) {
    const char* expected = expected_list[i];
    bool expected_found = false;
    for (int j = 0; j < list_length; ++j) {
      char* entry = list[j];
      if (strcmp(expected, entry) == 0) {
        TEST_CHECK(!expected_found);
        TEST_MSG("%s was found multiple times in %s", expected, list_joined);
        expected_found = true;
      }
    }
    TEST_CHECK(expected_found);
    TEST_MSG("%s not found in %s", expected, list_joined);
  }
  free(list_joined);
  string_list_free(list, list_length);

  rmdir(tmp_dirname);
}

TEST_LIST = {
  {"file_does_exist", test_file_does_exist},
  {"file_size", test_file_size},
  {"file_find_one_with_prefix", test_file_find_one_with_prefix},
  {"file_find_one_with_suffix", test_file_find_one_with_suffix},
  {"file_join_paths", test_file_join_paths},
  {"file_read", test_file_read},
  {"file_write", test_file_write},
  {"file_list_dir", test_file_list_dir},
  {NULL, NULL},
};