#include "app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "coqui-stt.h"

#include "audio_buffer.h"
#include "pa_list_devices.h"
#include "settings.h"
#include "string_utils.h"
#include "trace.h"
#include "wav_io.h"

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

static char* get_device_name(const char* source) {
  if (strcmp(source, "mic") == 0) {
    return NULL;
  }
  else if (strcmp(source, "system") == 0) {
    char** input_devices = NULL;
    int input_devices_length = 0;
    get_input_devices(&input_devices, &input_devices_length);
    char* result = NULL;
    for (int i = 0; i < input_devices_length; ++i) {
      char* input_device = input_devices[i];
      if (string_ends_with(input_device, ".monitor")) {
        result = string_duplicate(input_device);
        break;
      }
    }
    if (result == NULL) {
      fprintf(stderr, "System source was specified, but none was found.\n");
    }
    return result;
  }
  else {
    return string_duplicate(source);
  }
}

static char* plain_text_from_transcript(const CandidateTranscript* transcript) {
  char* result = string_duplicate("");
  float previous_time = 0.0f;
  for (int i = 0; i < transcript->num_tokens; ++i) {
    const TokenMetadata* token = &transcript->tokens[i];
    const float current_time = token->start_time;
    const float time_since_previous = current_time - previous_time;
    if (time_since_previous > 1.0f) {
      result = string_append_in_place(result, "\n");
      if (strcmp(token->text, " ") != 0) {
        result = string_append_in_place(result, token->text);
      }
    }
    else {
      result = string_append_in_place(result, token->text);
    }
    previous_time = current_time;
  }
  return result;
}

static void output_streaming_transcript(const Metadata* current_metadata,
  const Metadata* previous_metadata) {
  const CandidateTranscript* current_transcript =
    &current_metadata->transcripts[0];
  char* current_text = plain_text_from_transcript(current_transcript);
  char* previous_text;
  if (previous_metadata == NULL) {
    previous_text = string_duplicate("");
  }
  else {
    const CandidateTranscript* previous_transcript =
      &previous_metadata->transcripts[0];
    previous_text = plain_text_from_transcript(previous_transcript);
  }
  if (strcmp(current_text, previous_text) != 0) {
    fprintf(stdout, "%s\n", current_text);
  }
  free(current_text);
  free(previous_text);
}

static bool process_file(const Settings* settings, ModelState* model_state,
  const char* filename) {
  AudioBuffer* buffer = NULL;
  if (!wav_io_load(filename, &buffer)) {
    return false;
  }
  Metadata* metadata = STT_SpeechToTextWithMetadata(model_state, buffer->data,
    buffer->samples_per_channel, 1);
  output_streaming_transcript(metadata, NULL);
  audio_buffer_free(buffer);
  return true;
}

static bool process_files(const Settings* settings, ModelState* model_state) {
  for (int i = 0; i < settings->files_count; ++i) {
    if (!process_file(settings, model_state, settings->files[i])) {
      return false;
    }
  }
  return true;
}

static bool process_live_input(const Settings* settings, ModelState* model_state) {
  char* device_name = get_device_name(settings->source);

  const uint32_t model_rate = STT_GetModelSampleRate(model_state);
  const pa_sample_spec sample_spec = { PA_SAMPLE_S16LE, model_rate, 1 };
  int pa_error;
  pa_simple* source_stream = pa_simple_new(
    NULL, yargs_app_name(), PA_STREAM_RECORD, device_name, yargs_app_name(),
    &sample_spec, NULL, NULL, &pa_error);
  if (source_stream == NULL) {
    if (device_name == NULL) {
      fprintf(stderr, "Unable to open default audio input device.\n");
    }
    else {
      fprintf(stderr,
        "Unable to open audio input device named '%s', from source '%s'.\n",
        device_name, settings->source);
    }
    fprintf(stderr, "The command 'pactl list sources' will show available devices.\n");
    fprintf(stderr, "You can use the contents of the 'Name:' field as the '--source' argument to specify one.\n");
    free(device_name);
    return false;
  }

  StreamingState* streaming_state = NULL;
  const int stream_error = STT_CreateStream(model_state, &streaming_state);
  if (stream_error != STT_ERR_OK) {
    const char* error_message = STT_ErrorCodeToErrorMessage(stream_error);
    fprintf(stderr, "STT_CreateStream() failed with '%s'\n", error_message);
    pa_simple_free(source_stream);
    free(device_name);
    return false;
  }

  const size_t source_buffer_byte_count = settings->source_buffer_size * 2;
  int16_t* source_buffer = malloc(source_buffer_byte_count);

  AudioBuffer* capture_buffer = NULL;
  if (settings->stream_capture_file != NULL) {
    capture_buffer =
      audio_buffer_alloc(model_rate, settings->stream_capture_duration, 1);
  }
  int stream_capture_offset = 0;

  Metadata* previous_metadata = NULL;
  while (true) {
    int read_error;
    const int read_result = pa_simple_read(source_stream, source_buffer,
      source_buffer_byte_count, &read_error);
    if (read_result < 0) {
      fprintf(stderr, "pa_simple_read() failed with '%s'.\n",
        pa_strerror(read_error));
      break;
    }
    if (capture_buffer != NULL) {
      if ((stream_capture_offset + settings->source_buffer_size) > settings->stream_capture_duration) {
        break;
      }
      int16_t* current_capture = capture_buffer->data + stream_capture_offset;
      memcpy(current_capture, source_buffer, source_buffer_byte_count);
      stream_capture_offset += settings->source_buffer_size;
    }

    STT_FeedAudioContent(streaming_state, source_buffer,
      settings->source_buffer_size);
    Metadata* current_metadata = STT_IntermediateDecodeWithMetadata(streaming_state, 1);

    output_streaming_transcript(current_metadata, previous_metadata);

    if (previous_metadata != NULL) {
      STT_FreeMetadata(previous_metadata);
    }
    previous_metadata = current_metadata;
  }

  if (capture_buffer != NULL) {
    wav_io_save(settings->stream_capture_file, capture_buffer);
    audio_buffer_free(capture_buffer);
  }

  if (previous_metadata != NULL) {
    STT_FreeMetadata(previous_metadata);
  }
  pa_simple_free(source_stream);
  free(device_name);
  return true;
}

static bool process_audio(const Settings* settings, ModelState* model_state) {
  if (strcmp(settings->source, "file") == 0) {
    return process_files(settings, model_state);
  }
  else {
    return process_live_input(settings, model_state);
  }
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

  if (!process_audio(settings, model_state)) {
    return 1;
  }

  STT_FreeModel(model_state);

  return 0;
}