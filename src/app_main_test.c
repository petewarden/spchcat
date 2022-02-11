#include "acutest.h"

#include "app_main.c"

#include "file_utils.h"

void test_plain_text_from_transcript() {
  TokenMetadata tokens1[] = {
    {"h", 50, 1.0f},
    {"e", 55, 1.1f},
    {"l", 60, 1.2f},
    {"l", 65, 1.3f},
    {"o", 70, 1.4f},
    {" ", 80, 1.6f},
    {"w", 85, 1.7f},
    {"o", 90, 1.8f},
    {"r", 95, 1.9f},
    {"l", 100, 2.0f},
    {"d", 105, 2.1f},
  };
  const int tokens1_length = sizeof(tokens1) / sizeof(tokens1[0]);
  CandidateTranscript transcript1 = {
    tokens1, tokens1_length, 1.0f,
  };
  char* result = plain_text_from_transcript(&transcript1);
  TEST_ASSERT(result != NULL);
  TEST_STREQ("hello world", result);
  free(result);

  TokenMetadata tokens2[] = {
    {"h", 50, 1.0f},
    {"e", 55, 1.1f},
    {"l", 60, 1.2f},
    {"l", 65, 1.3f},
    {"o", 70, 1.4f},
    {" ", 500, 10.0f},
    {"w", 505, 10.1f},
    {"o", 510, 10.2f},
    {"r", 515, 10.3f},
    {"l", 520, 10.4f},
    {"d", 525, 10.5f},
  };
  const int tokens2_length = sizeof(tokens2) / sizeof(tokens2[0]);
  CandidateTranscript transcript2 = {
    tokens2, tokens2_length, 1.0f,
  };
  result = plain_text_from_transcript(&transcript2);
  TEST_ASSERT(result != NULL);
  TEST_STREQ("hello\nworld", result);
  free(result);

  TokenMetadata tokens3[] = {
    {"h", 50, 1.0f},
    {"e", 55, 1.1f},
    {"l", 60, 1.2f},
    {"l", 65, 1.3f},
    {"o", 70, 1.4f},
    {" ", 75, 1.5f},
    {"w", 505, 10.1f},
    {"o", 510, 10.2f},
    {"r", 515, 10.3f},
    {"l", 520, 10.4f},
    {"d", 525, 10.5f},
  };
  const int tokens3_length = sizeof(tokens3) / sizeof(tokens3[0]);
  CandidateTranscript transcript3 = {
    tokens3, tokens3_length, 1.0f,
  };
  result = plain_text_from_transcript(&transcript3);
  TEST_ASSERT(result != NULL);
  TEST_STREQ("hello\nworld", result);
  free(result);
}

void test_print_changed_lines() {
  const char* test_filename = "/tmp/test_print_changed_lines.txt";
  FILE* test_file = fopen(test_filename, "wb");
  TEST_ASSERT(test_file != NULL);
  const char* previous1 = "Hello";
  const char* current1 = "Hello World";
  print_changed_lines(current1, previous1, test_file);
  fclose(test_file);

  const char* expected = "\rHello World        ";
  const int expected_length = strlen(expected);
  char* result = NULL;
  size_t result_length = 0;
  TEST_ASSERT(file_read(test_filename, &result, &result_length));
  TEST_ASSERT_((expected_length == result_length),
    "%d vs %zu", expected_length, result_length);
  TEST_MEMEQ(expected, result, strlen(expected));
  free(result);

  test_file = fopen(test_filename, "wb");
  TEST_ASSERT(test_file != NULL);
  const char* previous2 = "";
  const char* current2 = "Hello World\nThis is Pete";
  print_changed_lines(current2, previous2, test_file);
  fclose(test_file);

  const char* expected2 = "\rHello World\n\rThis is Pete        ";
  const int expected2_length = strlen(expected2);
  result = NULL;
  result_length = 0;
  TEST_ASSERT(file_read(test_filename, &result, &result_length));
  TEST_CHECK(expected2_length == result_length);
  TEST_MSG("%d vs %zu", expected2_length, result_length);
  TEST_DUMP("result", result, result_length);
  TEST_ASSERT(expected2_length == result_length);
  TEST_MEMEQ(expected2, result, expected2_length);
  free(result);

  test_file = fopen(test_filename, "wb");
  TEST_ASSERT(test_file != NULL);
  const char* previous3 = "\rHello World        ";
  const char* current3 = "Hello World\nThis is Pete";
  print_changed_lines(current3, previous3, test_file);
  fclose(test_file);

  const char* expected3 = "\rHello World\n\rThis is Pete        ";
  const int expected3_length = strlen(expected3);
  result = NULL;
  result_length = 0;
  TEST_ASSERT(file_read(test_filename, &result, &result_length));
  TEST_CHECK(expected3_length == result_length);
  TEST_MSG("%d vs %zu", expected3_length, result_length);
  TEST_DUMP("result", result, result_length);
  TEST_ASSERT(expected3_length == result_length);
  TEST_MEMEQ(expected3, result, expected3_length);
  free(result);
}

TEST_LIST = {
  {"plain_text_from_transcript", test_plain_text_from_transcript},
  {"print_changed_lines", test_print_changed_lines},
  {NULL, NULL},
};