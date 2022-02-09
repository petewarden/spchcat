#include "app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coqui-stt.h"
#include "settings.h"
#include "string_utils.h"
#include "trace.h"

static bool load_model(const Settings* settings, ModelState** model_state) {
  const int create_status = STT_CreateModel(settings->model, model_state);
  if (create_status != 0) {
    char* error_message = STT_ErrorCodeToErrorMessage(create_status);
    fprintf(stderr, "STT_CreateModel failed with '%s' (%d)\n", error_message,
      create_status);
    free(error_message);
    return false;
  }

  if (settings->beam_width > 0) {
    const int beam_width_status = STT_SetModelBeamWidth(*model_state,
      settings->beam_width);
    if (beam_width_status != 0) {
      char* error_message = STT_ErrorCodeToErrorMessage(beam_width_status);
      fprintf(stderr, "STT_SetModelBeamWidth failed with '%s' (%d)\n", error_message,
        beam_width_status);
      free(error_message);
    }
  }

  return true;
}

static bool load_scorer(const Settings* settings, ModelState* model_state) {
  if (settings->scorer == NULL) {
    return true;
  }
  const int scorer_status = STT_EnableExternalScorer(model_state,
    settings->scorer);
  if (scorer_status != 0) {
    char* error_message = STT_ErrorCodeToErrorMessage(scorer_status);
    fprintf(stderr, "STT_EnableExternalScorer failed with '%s' (%d)\n", error_message,
      scorer_status);
    free(error_message);
    return false;
  }
  if (settings->lm_alpha > 0.0f) {
    const int alpha_status = STT_SetScorerAlphaBeta(model_state,
      settings->lm_alpha, settings->lm_beta);
    if (alpha_status != 0) {
      char* error_message = STT_ErrorCodeToErrorMessage(alpha_status);
      fprintf(stderr, "STT_SetScorerAlphaBeta failed with '%s' (%d)\n", error_message,
        alpha_status);
      free(error_message);
      return false;
    }
  }
  if (settings->hot_words) {
    char** parts = NULL;
    int parts_length = 0;
    string_split(settings->hot_words, ',', -1, &parts, &parts_length);
    for (int i = 0; i < parts_length; ++i) {
      char* part = parts[i];
      char** entry_parts = NULL;
      int entry_parts_length = 0;
      string_split(part, ':', 2, &entry_parts, &entry_parts_length);
      if (entry_parts_length != 2) {
        fprintf(stderr,
          "Expected format 'word:number' in --hotwords but found '%s'.\n",
          part);
        string_list_free(entry_parts, entry_parts_length);
        string_list_free(parts, parts_length);
        return false;
      }
      char* hot_word = entry_parts[0];
      char* boost_string = entry_parts[1];
      char* conversion_end = NULL;
      const float boost = strtof(boost_string, &conversion_end);
      const int converted_length = (conversion_end - boost_string);
      if (converted_length != strlen(boost_string)) {
        fprintf(stderr,
          "Expected format 'word:number' in --hotwords but found '%s'.\n",
          part);
        string_list_free(entry_parts, entry_parts_length);
        string_list_free(parts, parts_length);
        return false;
      }
      const int hot_word_status = STT_AddHotWord(model_state, hot_word, boost);
      if (hot_word_status != 0) {
        char* error_message = STT_ErrorCodeToErrorMessage(hot_word_status);
        fprintf(stderr, "STT_AddHotWord failed with '%s' (%d)\n", error_message,
          hot_word_status);
        string_list_free(entry_parts, entry_parts_length);
        string_list_free(parts, parts_length);
        free(error_message);
        return false;
      }
      string_list_free(entry_parts, entry_parts_length);
    }
    string_list_free(parts, parts_length);
  }

  return true;
}

int app_main(int argc, char** argv) {
  Settings* settings = settings_init_from_argv(argc, argv);
  if (settings == NULL) {
    return 1;
  }

  ModelState* model_state = NULL;
  if (!load_model(settings, &model_state)) {
    return 1;
  }

  if (!load_scorer(settings, model_state)) {
    return 1;
  }

  STT_FreeModel(model_state);

  return 0;
}