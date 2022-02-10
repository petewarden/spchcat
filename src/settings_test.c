#include "settings.h"

#include "acutest.h"

#include "settings.c"

#include <stdlib.h>

// Not sure why this declaration isn't pulled in?
extern int setenv(const char*, const char*, int);

static void create_mock_languages_dir(const char* root, const char** dirs,
  int dirs_length) {
  mkdir(root, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  for (int i = 0; i < dirs_length; ++i) {
    char* dirpath = file_join_paths(root, dirs[i]);
    mkdir(dirpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    free(dirpath);
  }
}

void test_is_real_entry() {
  TEST_CHECK(is_real_entry("foo", NULL));
  TEST_CHECK(!is_real_entry(".", NULL));
  TEST_CHECK(!is_real_entry("..", NULL));
}

void test_available_languages() {
  const char* languages_dir = "/tmp/test_lang_dir";
  const char* languages[] = {
    "en_US", "es_ES", "fr_FR",
  };
  const int languages_length = sizeof(languages) / sizeof(languages[0]);
  create_mock_languages_dir(languages_dir, languages, languages_length);

  char* result = available_languages(languages_dir);
  TEST_STR_CONTAINS("en_US", result);
  TEST_STR_CONTAINS("es_ES", result);
  TEST_STR_CONTAINS("fr_FR", result);
  TEST_SIZEQ(19, strlen(result));

  free(result);
  rmdir(languages_dir);
}

void test_language_description() {
  const char* languages_dir = "/tmp/test_lang_dir2";
  const char* languages[] = {
    "en_US", "de_DE", "fr_FR",
  };
  const int languages_length = sizeof(languages) / sizeof(languages[0]);
  create_mock_languages_dir(languages_dir, languages, languages_length);

  Settings settings;
  settings.languages_dir = languages_dir;
  settings.language = string_duplicate("es_ES");
  char* result = language_description(&settings);
  TEST_STR_CONTAINS("en_US", result);
  TEST_STR_CONTAINS("de_DE", result);
  TEST_STR_CONTAINS("fr_FR", result);
  TEST_STR_CONTAINS("es_ES", result);

  free(result);
  rmdir(languages_dir);
  free(settings.language);
}

void test_set_defaults() {
  const char* old_lang = getenv("LANG");

  Settings settings1 = {};
  setenv("LANG", "en_UK.UTF-8", 1);
  set_defaults(&settings1);
  TEST_STREQ("en_UK", settings1.language);
  free(settings1.language);

  Settings settings2 = {};
  setenv("LANG", "", 1);
  set_defaults(&settings2);
  TEST_STREQ("en_US", settings2.language);
  free(settings2.language);

  setenv("LANG", old_lang, 1);
}

void test_find_model_for_language() {
  const char* languages_dir = "/tmp/test_lang_dir3";
  const char* languages[] = {
    "en_US", "de_DE", "de_AT", "fr_FR",
  };
  const int languages_length = sizeof(languages) / sizeof(languages[0]);
  create_mock_languages_dir(languages_dir, languages, languages_length);
  char* en_us_dir = file_join_paths(languages_dir, "en_US");
  char* en_us_model = file_join_paths(en_us_dir, "model.tflite");
  free(en_us_dir);
  file_write(en_us_model, "a\0", 2);

  Settings* settings1 = calloc(sizeof(Settings), 1);
  settings1->model = "/foo/bar/baz.tflite";
  find_model_for_language(settings1);
  TEST_STREQ("/foo/bar/baz.tflite", settings1->model);
  settings_free(settings1);

  Settings* settings2 = calloc(sizeof(Settings), 1);
  settings2->language_from_args = "en_US";
  settings2->languages_dir = languages_dir;
  find_model_for_language(settings2);
  TEST_STREQ(en_us_model, settings2->model);
  settings_free(settings2);

  Settings* settings3 = calloc(sizeof(Settings), 1);
  settings3->language_from_args = "en_UK";
  settings3->languages_dir = languages_dir;
  find_model_for_language(settings3);
  TEST_STREQ(en_us_model, settings3->model);
  settings_free(settings3);

  Settings* settings4 = calloc(sizeof(Settings), 1);
  settings4->language_from_args = "de_UK";
  settings4->languages_dir = languages_dir;
  find_model_for_language(settings4);
  TEST_CHECK(settings4->model == NULL);
  settings_free(settings4);

  free(en_us_model);
  rmdir(languages_dir);
}

void test_find_scorer_for_language() {
  const char* languages_dir = "/tmp/test_lang_dir3";
  const char* languages[] = {
    "en_US", "de_DE", "de_AT", "fr_FR",
  };
  const int languages_length = sizeof(languages) / sizeof(languages[0]);
  create_mock_languages_dir(languages_dir, languages, languages_length);
  char* en_us_dir = file_join_paths(languages_dir, "en_US");
  char* en_us_scorer = file_join_paths(en_us_dir, "some.scorer");
  free(en_us_dir);
  file_write(en_us_scorer, "a\0", 2);

  Settings* settings1 = calloc(sizeof(Settings), 1);
  settings1->scorer = "/foo/bar/baz.scorer";
  find_scorer_for_language(settings1);
  TEST_STREQ("/foo/bar/baz.scorer", settings1->scorer);
  settings_free(settings1);

  Settings* settings2 = calloc(sizeof(Settings), 1);
  settings2->language_from_args = "en_US";
  settings2->languages_dir = languages_dir;
  find_scorer_for_language(settings2);
  TEST_STREQ(en_us_scorer, settings2->model);
  settings_free(settings2);

  free(en_us_scorer);
  rmdir(languages_dir);
}

void test_set_source() {
  char* argv1[] = { "program" };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);
  Settings* settings1 = settings_init_from_argv(argc1, argv1);
  TEST_CHECK(settings1 != NULL);
  TEST_STREQ("mic", settings1->source);
  settings_free(settings1);

  char* argv2[] = { "program", "--source", "foo" };
  const int argc2 = sizeof(argv2) / sizeof(argv2[0]);
  Settings* settings2 = settings_init_from_argv(argc2, argv2);
  TEST_CHECK(settings2 != NULL);
  TEST_STREQ("foo", settings2->source);
  settings_free(settings2);

  char* argv3[] = { "program", "--source", "file", "foo.wav", "bar.wav" };
  const int argc3 = sizeof(argv3) / sizeof(argv3[0]);
  Settings* settings3 = settings_init_from_argv(argc3, argv3);
  TEST_CHECK(settings3 != NULL);
  TEST_STREQ("file", settings3->source);
  TEST_INTEQ(2, settings3->files_count);
  TEST_STREQ("foo.wav", settings3->files[0]);
  TEST_STREQ("bar.wav", settings3->files[1]);
  settings_free(settings3);

  char* argv4[] = { "program", "baz.wav", "fish.wav" };
  const int argc4 = sizeof(argv4) / sizeof(argv4[0]);
  Settings* settings4 = settings_init_from_argv(argc4, argv4);
  TEST_CHECK(settings4 != NULL);
  TEST_STREQ("file", settings4->source);
  TEST_INTEQ(2, settings4->files_count);
  TEST_STREQ("baz.wav", settings4->files[0]);
  TEST_STREQ("fish.wav", settings4->files[1]);
  settings_free(settings4);

  char* argv5[] = { "program", "--source=foo", "baz.wav", "fish.wav" };
  const int argc5 = sizeof(argv5) / sizeof(argv5[0]);
  Settings* settings5 = settings_init_from_argv(argc5, argv5);
  TEST_CHECK(settings5 == NULL);
  settings_free(settings5);

  char* argv6[] = { "program", "--source=file" };
  const int argc6 = sizeof(argv6) / sizeof(argv6[0]);
  Settings* settings6 = settings_init_from_argv(argc6, argv6);
  TEST_CHECK(settings6 == NULL);
  settings_free(settings6);
}

void test_settings_init_from_argv() {
  char* languages_dir = "/tmp/test_lang_dir3";
  const char* languages[] = {
    "en_US", "de_DE", "de_AT", "fr_FR",
  };
  const int languages_length = sizeof(languages) / sizeof(languages[0]);
  create_mock_languages_dir(languages_dir, languages, languages_length);
  char* en_us_dir = file_join_paths(languages_dir, "en_US");
  char* en_us_model = file_join_paths(en_us_dir, "model.tflite");
  free(en_us_dir);
  file_write(en_us_model, "a\0", 2);

  char* argv1[] = { "program" };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);
  Settings* settings1 = settings_init_from_argv(argc1, argv1);
  TEST_CHECK(settings1 != NULL);
  settings_free(settings1);

  char* argv2[] = { "program", "--unknown_flag" };
  const int argc2 = sizeof(argv2) / sizeof(argv2[0]);
  Settings* settings2 = settings_init_from_argv(argc2, argv2);
  TEST_CHECK(settings2 == NULL);

  char* argv3[] = { "program", "--model", "foo/bar/baz.tflite" };
  const int argc3 = sizeof(argv3) / sizeof(argv3[0]);
  Settings* settings3 = settings_init_from_argv(argc3, argv3);
  TEST_CHECK(settings3 != NULL);
  TEST_STREQ("foo/bar/baz.tflite", settings3->model);
  settings_free(settings3);

  char* argv4[] = { "program",
    "--language", "en",
    "--languages_dir", languages_dir,
  };
  const int argc4 = sizeof(argv4) / sizeof(argv4[0]);
  Settings* settings4 = settings_init_from_argv(argc4, argv4);
  TEST_CHECK(settings4 != NULL);
  TEST_STREQ(en_us_model, settings4->model);
  settings_free(settings4);

  char* argv5[] = { "program",
    "--language", "en_US",
    "--source", "file",
    "--languages_dir", languages_dir,
    "--scorer", "foo",
    "--source_buffer_size", "160",
    "--beam_width", "5",
    "--lm_alpha", "3.0",
    "--lm_beta", "4.0",
    "--show_times",
    "--has_versions",
    "--extended_metadata",
    "--json_output",
    "--json_candidate_transcripts", "3",
    "--stream_size", "320",
    "--extended_stream_size", "640",
    "--hot_words", "baz:10.0,fish:20.0",
    "--stream_capture_file", "/foo/bar.wav",
    "--stream_capture_duration", "32000",
    "file1", "file2",
  };
  const int argc5 = sizeof(argv5) / sizeof(argv5[0]);
  Settings* settings5 = settings_init_from_argv(argc5, argv5);
  TEST_CHECK(settings5 != NULL);
  TEST_STREQ("en_US", settings5->language);
  TEST_STREQ("file", settings5->source);
  TEST_STREQ(languages_dir, settings5->languages_dir);
  TEST_STREQ(en_us_model, settings5->model);
  TEST_STREQ("foo", settings5->scorer);
  TEST_INTEQ(160, settings5->source_buffer_size);
  TEST_INTEQ(5, settings5->beam_width);
  TEST_FLTEQ(3.0f, settings5->lm_alpha, 0.0001f);
  TEST_FLTEQ(4.0f, settings5->lm_beta, 0.0001f);
  TEST_CHECK(settings5->show_times);
  TEST_CHECK(settings5->has_versions);
  TEST_CHECK(settings5->extended_metadata);
  TEST_CHECK(settings5->json_output);
  TEST_INTEQ(3, settings5->json_candidate_transcripts);
  TEST_INTEQ(320, settings5->stream_size);
  TEST_INTEQ(640, settings5->extended_stream_size);
  TEST_STREQ("baz:10.0,fish:20.0", settings5->hot_words);
  TEST_STREQ("/foo/bar.wav", settings5->stream_capture_file);
  TEST_INTEQ(32000, settings5->stream_capture_duration);
  settings_free(settings5);

  free(en_us_model);
}

TEST_LIST = {
  {"is_real_entry", test_is_real_entry},
  {"available_languages", test_available_languages},
  {"test_language_description", test_language_description},
  {"test_set_defaults", test_set_defaults},
  {"test_find_model_for_language", test_find_model_for_language},
  {"test_set_source", test_set_source},
  {"test_settings_init_from_argv", test_settings_init_from_argv},
  {NULL, NULL},
};