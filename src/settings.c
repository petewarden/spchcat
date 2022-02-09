#include "settings.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_utils.h"
#include "string_utils.h"
#include "trace.h"
#include "yargs.h"

static bool is_real_entry(const char* name, void* cookie) {
  return ((strcmp(name, ".") != 0) &&
    (strcmp(name, "..") != 0));
}

static char* available_languages(const char* languages_dir) {
  char** dirs_list;
  int dirs_list_length;
  const bool list_status =
    file_list_dir(languages_dir, &dirs_list, &dirs_list_length);
  if (!list_status) {
    return string_duplicate("");
  }
  char** real_dirs_list;
  int real_dirs_list_length;
  string_list_filter((const char**)(dirs_list), dirs_list_length, is_real_entry, NULL,
    &real_dirs_list, &real_dirs_list_length);
  string_list_free(dirs_list, dirs_list_length);

  char* result = string_join((const char**)(real_dirs_list), real_dirs_list_length, ", ");
  string_list_free(real_dirs_list, real_dirs_list_length);
  return result;
}

static char* language_description(Settings* settings) {
  char* available_languages_string =
    available_languages(settings->languages_dir);
  char* result = string_alloc_sprintf("Which language to look for (default '"
    "%s', can be %s)", settings->language, available_languages_string);
  free(available_languages_string);
  return result;
}

static void set_defaults(Settings* settings) {
  const char* env_language = getenv("LANG");
  if ((env_language == NULL) || (strlen(env_language) == 0)) {
    settings->language = string_duplicate("en_US");
  }
  else {
    char** parts;
    int parts_length;
    string_split(env_language, '.', 2, &parts, &parts_length);
    settings->language = string_duplicate(parts[0]);
    string_list_free(parts, parts_length);
  }
  settings->language_from_args = NULL;
  settings->source = NULL;
  settings->languages_dir = "/etc/spchcat/models/";
  settings->model = NULL;
  settings->scorer = NULL;
  settings->source_buffer_size = 160 * 4;
  settings->beam_width = 0;
  settings->lm_alpha = 0.0f;
  settings->lm_beta = 0.0f;
  settings->show_times = false;
  settings->has_versions = false;
  settings->extended_metadata = false;
  settings->json_output = false;
  settings->json_candidate_transcripts = 3;
  settings->stream_size = 0;
  settings->extended_stream_size = 0;
  settings->hot_words = NULL;
}

static void find_model_for_language(Settings* settings) {
  // If the model filename was explicitly set on the command line, don't worry
  // about searching for it.
  if (settings->model != NULL) {
    // Make a copy of the string we got from the arg parsing, so that we can
    // free it ourselves, like the other paths below.
    settings->model = string_duplicate(settings->model);
    return;
  }

  // Override any language we've guessed from env variables with any args
  // specified on the command line.
  if (settings->language_from_args != NULL) {
    free(settings->language);
    settings->language = string_duplicate(settings->language_from_args);
  }

  // Look for the exact match to the language and country combination, and if
  // a model file exists at that path, use it.
  char* language_folder = file_join_paths(settings->languages_dir, settings->language);
  char* model_filename = file_find_one_with_suffix(language_folder, ".tflite");
  if (model_filename != NULL) {
    settings->model = file_join_paths(language_folder, model_filename);
    free(model_filename);
    free(language_folder);
    return;
  }
  free(language_folder);

  // If the right country wasn't found, try falling back to any folder
  // with the right language.
  char** lang_parts;
  int lang_parts_length;
  string_split(settings->language, '_', 2, &lang_parts, &lang_parts_length);
  char* lang_only = string_append(lang_parts[0], "_");
  string_list_free(lang_parts, lang_parts_length);
  char* lang_only_folder =
    file_find_one_with_prefix(settings->languages_dir, lang_only);
  free(lang_only);
  if (lang_only_folder == NULL) {
    fprintf(stderr, "Unable to find a language model for '%s' in '%s'",
      settings->language, settings->languages_dir);
    return;
  }
  char** path_parts;
  int path_parts_length;
  string_split(lang_only_folder, '/', -1, &path_parts,
    &path_parts_length);
  free(lang_only_folder);
  char* found_language =
    string_duplicate(path_parts[path_parts_length - 1]);
  string_list_free(path_parts, path_parts_length);
  char* found_language_folder =
    file_join_paths(settings->languages_dir, found_language);
  char* found_model_filename = file_find_one_with_suffix(found_language_folder,
    ".tflite");
  if (found_model_filename == NULL) {
    fprintf(stderr, "Unable to find a language model for '%s' in '%s'\n",
      settings->language, found_language_folder);
    free(found_language_folder);
    free(found_language);
    return;
  }
  fprintf(stderr, "Warning: Language '%s' not found, falling back to '%s'\n",
    settings->language, found_language);
  free(settings->language);
  settings->language = found_language;
  settings->model = file_join_paths(found_language_folder, found_model_filename);
  free(found_language_folder);
  free(found_model_filename);
}

static void find_scorer_for_language(Settings* settings) {
  // If the scorer filename was explicitly set on the command line, don't worry
  // about searching for it.
  if (settings->scorer != NULL) {
    // Make a copy of the string we got from the arg parsing, so that we can
    // free it ourselves, like the other paths below.
    settings->scorer = string_duplicate(settings->scorer);
    return;
  }

  char* language_folder =
    file_join_paths(settings->languages_dir, settings->language);

  char* scorer = file_find_one_with_suffix(language_folder, ".scorer");
  if ((scorer == NULL) ||
    (strstr(scorer, "command") != NULL) ||
    (strstr(scorer, "digit") != NULL) ||
    (strstr(scorer, "yesno") != NULL)) {
    // These are too small to be useful, so skip them.
    free(scorer);
    free(language_folder);
    return;
  }

  settings->scorer = file_join_paths(language_folder, scorer);
  free(scorer);
  free(language_folder);
}

static bool set_source(Settings* settings) {
  const int files_length = yargs_get_unnamed_length();
  if (settings->source != NULL) {
    if ((files_length != 0) &&
      (strcmp(settings->source, "file") != 0)) {
      fprintf(stderr,
        "Source '%s' was specified, but files were also passed as arguments.\n",
        settings->source);
      return false;
    }
    else if ((files_length == 0) &&
      (strcmp(settings->source, "file") == 0)) {
      fprintf(stderr,
        "File source was specified, but no files were passed as arguments.\n");
      return false;
    }
  }
  else if (files_length == 0) {
    settings->source = "mic";
  }
  else {
    settings->source = "file";
  }

  if (strcmp(settings->source, "file") == 0) {
    settings->files_count = files_length;
    settings->files = calloc(files_length, sizeof(char*));
    for (int i = 0; i < files_length; ++i) {
      settings->files[i] = string_duplicate(yargs_get_unnamed(i));
    }
  }
  else {
    settings->files_count = 0;
    settings->files = NULL;
  }

  return true;
}

Settings* settings_init_from_argv(int argc, char** argv) {

  Settings* settings = (Settings*)(calloc(1, sizeof(Settings)));
  set_defaults(settings);

  char* language_description_string = language_description(settings);
  YargsFlag language_flag = YARGS_STRING("language", "l",
    &settings->language_from_args, language_description_string);

  bool show_help = false;
  const YargsFlag flags[] = {
    YARGS_BOOL("help", "?", &show_help, "Displays usage information"),
    language_flag,
    YARGS_STRING("source", "s", &settings->source, ""),
    YARGS_STRING("hot_words", "h", &settings->hot_words, ""),
    YARGS_STRING("languages_dir", "d", &settings->languages_dir, ""),
    YARGS_STRING("model", "m", (const char**)(&settings->model), ""),
    YARGS_STRING("scorer", "c", (const char**)(&settings->scorer), ""),
    YARGS_INT32("source_buffer_size", "o", &settings->source_buffer_size, ""),
    YARGS_INT32("beam_width", "b", &settings->beam_width, ""),
    YARGS_FLOAT("lm_alpha", "a", &settings->lm_alpha, ""),
    YARGS_FLOAT("lm_beta", "e", &settings->lm_beta, ""),
    YARGS_BOOL("show_times", "t", &settings->show_times, ""),
    YARGS_BOOL("has_versions", "q", &settings->has_versions, ""),
    YARGS_BOOL("extended_metadata", "x", &settings->extended_metadata, ""),
    YARGS_BOOL("json_output", "j", &settings->json_output, ""),
    YARGS_INT32("json_candidate_transcripts", "n", &settings->json_candidate_transcripts, ""),
    YARGS_INT32("stream_size", "z", &settings->stream_size, ""),
    YARGS_INT32("extended_stream_size", "r", &settings->extended_stream_size, ""),
  };
  const int flags_length = sizeof(flags) / sizeof(flags[0]);

  const char* app_description =
    "Speech recognition tool to convert audio to text transcripts.";
  const bool init_status = yargs_init(flags, flags_length,
    app_description, argv, argc);
  if (!init_status) {
    free(language_description_string);
    settings_free(settings);
    return NULL;
  }

  if (show_help) {
    // Make sure we pick up any languages from a possibly-changed model path.
    free(language_flag.description);
    language_flag.description = language_description(settings);
    yargs_print_usage(flags, flags_length, app_description);
    yargs_free();
    free(language_flag.description);
    free(settings);
    return NULL;
  }

  find_model_for_language(settings);
  if (settings->model == NULL) {
    yargs_free();
    free(language_flag.description);
    free(settings);
    return NULL;
  }

  find_scorer_for_language(settings);

  if (!set_source(settings)) {
    yargs_free();
    free(language_flag.description);
    free(settings->language);
    free(settings->model);
    free(settings->scorer);
    free(settings);
    return NULL;
  }

  free(language_description_string);

  return settings;
}

void settings_free(Settings* settings) {
  yargs_free();
  if (settings == NULL) {
    return;
  }
  free(settings->language);
  free(settings->model);
  free(settings->scorer);
  string_list_free(settings->files, settings->files_count);
  free(settings);
}
