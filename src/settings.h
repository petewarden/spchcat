#ifndef INCLUDE_SETTINGS_H
#define INCLUDE_SETTINGS_H

#include <stdbool.h>

#include "yargs.h"

#ifdef __CPLUSPLUS
extern "C" {
#endif  // __CPLUSPLUS

  typedef struct SettingsStruct {
    char* language;
    const char* language_from_args;
    const char* source;
    const char* languages_dir;
    char* model;
    char* scorer;
    int source_buffer_size;
    int beam_width;
    bool set_alphabeta;
    float lm_alpha;
    float lm_beta;
    bool show_times;
    bool has_versions;
    bool extended_metadata;
    bool json_output;
    int json_candidate_transcripts;
    int stream_size;
    int extended_stream_size;
    const char* hot_words;
    const char* stream_capture_file;
    int stream_capture_duration;
    char** files;
    int files_count;
  } Settings;

  Settings* settings_init_from_argv(int argc, char** argv);
  void settings_free(Settings* settings);

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_SETTINGS_H