#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sstream>
#include <string>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__ANDROID__) || defined(_MSC_VER) || TARGET_OS_IPHONE
#define NO_SOX
#endif

#if defined(_MSC_VER)
#define NO_DIR
#endif

#ifndef NO_SOX
#include <sox.h>
#endif

#ifndef NO_DIR
#include <dirent.h>
#include <unistd.h>
#endif // NO_DIR
#include <vector>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "coqui-stt.h"

#include "args.h"
#include "pa_list_devices.h"

typedef struct
{
  const char* string;
  double cpu_time_overall;
} ds_result;

struct meta_word
{
  std::string word;
  float start_time;
  float duration;
};

char*
CandidateTranscriptToString(const CandidateTranscript* transcript)
{
  std::string retval = "";
  for (int i = 0; i < transcript->num_tokens; i++)
  {
    const TokenMetadata& token = transcript->tokens[i];
    retval += token.text;
  }
  return strdup(retval.c_str());
}

std::vector<meta_word>
CandidateTranscriptToWords(const CandidateTranscript* transcript)
{
  std::vector<meta_word> word_list;

  std::string word = "";
  float word_start_time = 0;

  // Loop through each token
  for (int i = 0; i < transcript->num_tokens; i++)
  {
    const TokenMetadata& token = transcript->tokens[i];

    // Append token to word if it's not a space
    if (strcmp(token.text, u8" ") != 0)
    {
      // Log the start time of the new word
      if (word.length() == 0)
      {
        word_start_time = token.start_time;
      }
      word.append(token.text);
    }

    // Word boundary is either a space or the last token in the array
    if (strcmp(token.text, u8" ") == 0 || i == transcript->num_tokens - 1)
    {
      float word_duration = token.start_time - word_start_time;

      if (word_duration < 0)
      {
        word_duration = 0;
      }

      meta_word w;
      w.word = word;
      w.start_time = word_start_time;
      w.duration = word_duration;

      word_list.push_back(w);

      // Reset
      word = "";
      word_start_time = 0;
    }
  }

  return word_list;
}

std::string
CandidateTranscriptToFormattedString(const CandidateTranscript* transcript)
{
  std::string result;

  std::string word = "";
  float word_start_time = 0;
  float previous_end_time = 0;

  // Loop through each token
  for (int i = 0; i < transcript->num_tokens; i++)
  {
    const TokenMetadata& token = transcript->tokens[i];

    // Append token to word if it's not a space
    if (strcmp(token.text, u8" ") != 0)
    {
      // Log the start time of the new word
      if (word.length() == 0)
      {
        word_start_time = token.start_time;
        const float time_since_previous = word_start_time - previous_end_time;
        if (time_since_previous > 1.0) {
          result += "\n";
        }
      }
      word.append(token.text);
      previous_end_time = token.start_time;
    }

    // Word boundary is either a space or the last token in the array
    if (strcmp(token.text, u8" ") == 0 || i == transcript->num_tokens - 1)
    {
      result += word + " ";
      //fprintf(stderr, "%f\n", previous_end_time);
      // Reset
      word = "";
    }
  }

  return result;
}

std::string
CandidateTranscriptToJSON(const CandidateTranscript* transcript)
{
  std::ostringstream out_string;

  std::vector<meta_word> words = CandidateTranscriptToWords(transcript);

  out_string << R"("metadata":{"confidence":)" << transcript->confidence << R"(},"words":[)";

  for (int i = 0; i < words.size(); i++)
  {
    meta_word w = words[i];
    out_string << R"({"word":")" << w.word << R"(","time":)" << w.start_time << R"(,"duration":)" << w.duration << "}";

    if (i < words.size() - 1)
    {
      out_string << ",";
    }
  }

  out_string << "]";

  return out_string.str();
}

char*
MetadataToJSON(Metadata* result)
{
  std::ostringstream out_string;
  out_string << "{\n";

  for (int j = 0; j < result->num_transcripts; ++j)
  {
    const CandidateTranscript* transcript = &result->transcripts[j];

    if (j == 0)
    {
      out_string << CandidateTranscriptToJSON(transcript);

      if (result->num_transcripts > 1)
      {
        out_string << ",\n"
          << R"("alternatives")"
          << ":[\n";
      }
    }
    else
    {
      out_string << "{" << CandidateTranscriptToJSON(transcript) << "}";

      if (j < result->num_transcripts - 1)
      {
        out_string << ",\n";
      }
      else
      {
        out_string << "\n]";
      }
    }
  }

  out_string << "\n}\n";

  return strdup(out_string.str().c_str());
}

ds_result
LocalDsSTT(ModelState* aCtx, const short* aBuffer, size_t aBufferSize,
  bool extended_output, bool json_output)
{
  ds_result res = { 0 };

  clock_t ds_start_time = clock();

  // sphinx-doc: c_ref_inference_start
  if (extended_output)
  {
    Metadata* result = STT_SpeechToTextWithMetadata(aCtx, aBuffer, aBufferSize, 1);
    res.string = CandidateTranscriptToString(&result->transcripts[0]);
    STT_FreeMetadata(result);
  }
  else if (json_output)
  {
    Metadata* result = STT_SpeechToTextWithMetadata(aCtx, aBuffer, aBufferSize, json_candidate_transcripts);
    res.string = MetadataToJSON(result);
    STT_FreeMetadata(result);
  }
  else if (stream_size > 0)
  {
    StreamingState* ctx;
    int status = STT_CreateStream(aCtx, &ctx);
    if (status != STT_ERR_OK)
    {
      res.string = strdup("");
      return res;
    }
    size_t off = 0;
    const char* last = nullptr;
    const char* prev = nullptr;
    while (off < aBufferSize)
    {
      size_t cur = aBufferSize - off > stream_size ? stream_size : aBufferSize - off;
      STT_FeedAudioContent(ctx, aBuffer + off, cur);
      off += cur;
      prev = last;
      const char* partial = STT_IntermediateDecode(ctx);
      if (last == nullptr || strcmp(last, partial))
      {
        printf("%s\n", partial);
        last = partial;
      }
      else
      {
        STT_FreeString((char*)partial);
      }
      if (prev != nullptr && prev != last)
      {
        STT_FreeString((char*)prev);
      }
    }
    if (last != nullptr)
    {
      STT_FreeString((char*)last);
    }
    res.string = STT_FinishStream(ctx);
  }
  else if (extended_stream_size > 0)
  {
    StreamingState* ctx;
    int status = STT_CreateStream(aCtx, &ctx);
    if (status != STT_ERR_OK)
    {
      res.string = strdup("");
      return res;
    }
    size_t off = 0;
    const char* last = nullptr;
    const char* prev = nullptr;
    while (off < aBufferSize)
    {
      size_t cur = aBufferSize - off > extended_stream_size ? extended_stream_size : aBufferSize - off;
      STT_FeedAudioContent(ctx, aBuffer + off, cur);
      off += cur;
      prev = last;
      const Metadata* result = STT_IntermediateDecodeWithMetadata(ctx, 1);
      const char* partial = CandidateTranscriptToString(&result->transcripts[0]);
      if (last == nullptr || strcmp(last, partial))
      {
        printf("%s\n", partial);
        last = partial;
      }
      else
      {
        free((char*)partial);
      }
      if (prev != nullptr && prev != last)
      {
        free((char*)prev);
      }
      STT_FreeMetadata((Metadata*)result);
    }
    const Metadata* result = STT_FinishStreamWithMetadata(ctx, 1);
    res.string = CandidateTranscriptToString(&result->transcripts[0]);
    STT_FreeMetadata((Metadata*)result);
    free((char*)last);
  }
  else
  {
    const Metadata* metadata = STT_SpeechToTextWithMetadata(aCtx, aBuffer, aBufferSize, 1);
    std::string result_string = CandidateTranscriptToFormattedString(&metadata->transcripts[0]);
    // Make a copy so it's not on the stack. TODO: Fix leak.
    std::string* new_string = new std::string(result_string);
    res.string = new_string->c_str();
    STT_FreeMetadata((Metadata*)metadata);
  }
  // sphinx-doc: c_ref_inference_stop

  clock_t ds_end_infer = clock();

  res.cpu_time_overall =
    ((double)(ds_end_infer - ds_start_time)) / CLOCKS_PER_SEC;

  return res;
}

typedef struct
{
  char* buffer;
  size_t buffer_size;
} ds_audio_buffer;

ds_audio_buffer
GetAudioBuffer(const char* path, int desired_sample_rate)
{
  ds_audio_buffer res = { 0 };

#ifndef NO_SOX
  sox_format_t* input = sox_open_read(path, NULL, NULL, NULL);
  assert(input);

  // Resample/reformat the audio so we can pass it through the MFCC functions
  sox_signalinfo_t target_signal = {
      static_cast<sox_rate_t>(desired_sample_rate), // Rate
      1,                                            // Channels
      16,                                           // Precision
      SOX_UNSPEC,                                   // Length
      NULL                                          // Effects headroom multiplier
  };

  sox_signalinfo_t interm_signal;

  sox_encodinginfo_t target_encoding = {
      SOX_ENCODING_SIGN2, // Sample format
      16,                 // Bits per sample
      0.0,                // Compression factor
      sox_option_default, // Should bytes be reversed
      sox_option_default, // Should nibbles be reversed
      sox_option_default, // Should bits be reversed (pairs of bits?)
      sox_false           // Reverse endianness
  };

#if TARGET_OS_OSX
  // It would be preferable to use sox_open_memstream_write here, but OS-X
  // doesn't support POSIX 2008, which it requires. See Issue #461.
  // Instead, we write to a temporary file.
  char* output_name = tmpnam(NULL);
  assert(output_name);
  sox_format_t* output = sox_open_write(output_name, &target_signal,
    &target_encoding, "raw", NULL, NULL);
#else
  sox_format_t* output = sox_open_memstream_write(&res.buffer,
    &res.buffer_size,
    &target_signal,
    &target_encoding,
    "raw", NULL);
#endif

  assert(output);

  if ((int)input->signal.rate < desired_sample_rate)
  {
    fprintf(stderr, "Warning: original sample rate (%d) is lower than %dkHz. "
      "Up-sampling might produce erratic speech recognition.\n",
      desired_sample_rate, (int)input->signal.rate);
  }

  // Setup the effects chain to decode/resample
  char* sox_args[10];
  sox_effects_chain_t* chain =
    sox_create_effects_chain(&input->encoding, &output->encoding);

  interm_signal = input->signal;

  sox_effect_t* e = sox_create_effect(sox_find_effect("input"));
  sox_args[0] = (char*)input;
  assert(sox_effect_options(e, 1, sox_args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &input->signal) ==
    SOX_SUCCESS);
  free(e);

  e = sox_create_effect(sox_find_effect("rate"));
  assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &output->signal) ==
    SOX_SUCCESS);
  free(e);

  e = sox_create_effect(sox_find_effect("channels"));
  assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &output->signal) ==
    SOX_SUCCESS);
  free(e);

  e = sox_create_effect(sox_find_effect("output"));
  sox_args[0] = (char*)output;
  assert(sox_effect_options(e, 1, sox_args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &output->signal) ==
    SOX_SUCCESS);
  free(e);

  // Finally run the effects chain
  sox_flow_effects(chain, NULL, NULL);
  sox_delete_effects_chain(chain);

  // Close sox handles
  sox_close(output);
  sox_close(input);
#endif // NO_SOX

#ifdef NO_SOX
  // FIXME: Hack and support only mono 16-bits PCM with standard SoX header
  FILE* wave = fopen(path, "r");

  size_t rv;

  unsigned short audio_format;
  fseek(wave, 20, SEEK_SET);
  rv = fread(&audio_format, 2, 1, wave);

  unsigned short num_channels;
  fseek(wave, 22, SEEK_SET);
  rv = fread(&num_channels, 2, 1, wave);

  unsigned int sample_rate;
  fseek(wave, 24, SEEK_SET);
  rv = fread(&sample_rate, 4, 1, wave);

  unsigned short bits_per_sample;
  fseek(wave, 34, SEEK_SET);
  rv = fread(&bits_per_sample, 2, 1, wave);

  assert(audio_format == 1);                  // 1 is PCM
  assert(num_channels == 1);                  // MONO
  assert(sample_rate == desired_sample_rate); // at desired sample rate
  assert(bits_per_sample == 16);              // 16 bits per sample

  fprintf(stderr, "audio_format=%d\n", audio_format);
  fprintf(stderr, "num_channels=%d\n", num_channels);
  fprintf(stderr, "sample_rate=%d (desired=%d)\n", sample_rate, desired_sample_rate);
  fprintf(stderr, "bits_per_sample=%d\n", bits_per_sample);

  fseek(wave, 40, SEEK_SET);
  rv = fread(&res.buffer_size, 4, 1, wave);
  fprintf(stderr, "res.buffer_size=%ld\n", res.buffer_size);

  fseek(wave, 44, SEEK_SET);
  res.buffer = (char*)malloc(sizeof(char) * res.buffer_size);
  rv = fread(res.buffer, sizeof(char), res.buffer_size, wave);

  fclose(wave);
#endif // NO_SOX

#if TARGET_OS_OSX
  res.buffer_size = (size_t)(output->olength * 2);
  res.buffer = (char*)malloc(sizeof(char) * res.buffer_size);
  FILE* output_file = fopen(output_name, "rb");
  assert(fread(res.buffer, sizeof(char), res.buffer_size, output_file) == res.buffer_size);
  fclose(output_file);
  unlink(output_name);
#endif

  return res;
}

void ProcessFile(ModelState* context, const char* path, bool show_times)
{
  ds_audio_buffer audio = GetAudioBuffer(path, STT_GetModelSampleRate(context));

  // Pass audio to STT
  // We take half of buffer_size because buffer is a char* while
  // LocalDsSTT() expected a short*
  ds_result result = LocalDsSTT(context,
    (const short*)audio.buffer,
    audio.buffer_size / 2,
    extended_metadata,
    json_output);
  free(audio.buffer);

  if (result.string)
  {
    printf("%s\n", result.string);
    STT_FreeString((char*)result.string);
  }

  if (show_times)
  {
    printf("cpu_time_overall=%.05f\n",
      result.cpu_time_overall);
  }
}

std::vector<std::string>
SplitStringOnDelim(std::string in_string, std::string delim)
{
  std::vector<std::string> out_vector;
  char* tmp_str = new char[in_string.size() + 1];
  std::copy(in_string.begin(), in_string.end(), tmp_str);
  tmp_str[in_string.size()] = '\0';
  const char* token = strtok(tmp_str, delim.c_str());
  while (token != NULL)
  {
    out_vector.push_back(token);
    token = strtok(NULL, delim.c_str());
  }
  delete[] tmp_str;
  return out_vector;
}

void TranscribeFiles(ModelState* ctx, const char* audio)
{
#ifndef NO_SOX
  // Initialise SOX
  assert(sox_init() == SOX_SUCCESS);
#endif

  struct stat wav_info;
  if (0 != stat(audio, &wav_info))
  {
    fprintf(stderr, "File not found: %s\n", audio);
    return;
  }

  switch (wav_info.st_mode & S_IFMT)
  {
#ifndef _MSC_VER
  case S_IFLNK:
#endif
  case S_IFREG:
    ProcessFile(ctx, audio, show_times);
    break;

#ifndef NO_DIR
  case S_IFDIR:
  {
    fprintf(stderr, "Running on directory %s\n", audio);
    DIR* wav_dir = opendir(audio);
    assert(wav_dir);

    struct dirent* entry;
    while ((entry = readdir(wav_dir)) != NULL)
    {
      std::string fname = std::string(entry->d_name);
      if ((fname.find(".wav") == std::string::npos) ||
        (fname[0] == '.'))
      {
        continue;
      }

      std::ostringstream fullpath;
      fullpath << audio << "/" << fname;
      std::string path = fullpath.str();
      fprintf(stderr, "> %s\n", path.c_str());
      ProcessFile(ctx, path.c_str(), show_times);
    }
    closedir(wav_dir);
  }
  break;
#endif

  default:
    fprintf(stderr, "Unexpected type for %s: %d\n", audio, (wav_info.st_mode & S_IFMT));
    break;
  }

#ifndef NO_SOX
  // Deinitialise and quit
  sox_quit();
#endif // NO_SOX
}

int TranscribeLiveSource(ModelState* ctx, const char* source_name)
{
  const pa_sample_spec ss = {
      .format = PA_SAMPLE_S16LE,
      .rate = (uint32_t)(STT_GetModelSampleRate(ctx)),
      .channels = 1 };

  const char* device;
  if (strcmp(source_name, "mic") == 0)
  {
    device = NULL;
  }
  else if (strcmp(source_name, "system") == 0)
  {
    // Use a simplistic algorithm that assumes the first device name ending in
    // .monitor is the system audio source.
    static std::vector<std::string> input_devices = ListAudioInputDevices();
    device = NULL;
    for (std::string& input_name : input_devices) {
      if (HasEnding(input_name, ".monitor")) {
        device = input_name.c_str();
        break;
      }
    }
    if (device == NULL) {
      fprintf(stderr, "System audio requested, but no device found, defaulting to microphone. Available are:\n");
      for (std::string& input_name : input_devices) {
        fprintf(stderr, "%s\n", input_name.c_str());
      }
    }
  }
  else
  {
    device = source_name;
  }

  const bool is_interactive = isatty(fileno(stdout));

  pa_simple* source_stream = NULL;
  int result = 1;
  int error;

  const size_t buffer_byte_count = source_buffer_size * 2;
  const short* mic_buffer = (const short*)(malloc(buffer_byte_count));
  std::string previous;
  std::string previous_print;

  StreamingState* stream_ctx = nullptr;

  if (!(source_stream = pa_simple_new(
    NULL, app_name, PA_STREAM_RECORD, device, "spchcat", &ss, NULL, NULL, &error)))
  {
    fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n", pa_strerror(error));
    fprintf(stderr, "You may see this error if no microphone or audio source is found.\n");
    fprintf(stderr, "The command 'pactl list sources' will show which devices PulseAudio is aware of.\n");
    fprintf(stderr, "You can use the contents of the 'Name:' field as the '--source' argument to specify one.\n");
    goto finish;
  }

  error = STT_CreateStream(ctx, &stream_ctx);
  if (error != STT_ERR_OK)
  {
    char* error_message = STT_ErrorCodeToErrorMessage(error);
    fprintf(stderr, __FILE__ ": STT_CreateStream() failed: %s\n", error_message);
    goto finish;
  }

  while (true)
  {
    if (pa_simple_read(source_stream, (void*)(mic_buffer), buffer_byte_count, &error) < 0)
    {
      fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n", pa_strerror(error));
      goto finish;
    }

    STT_FeedAudioContent(stream_ctx, mic_buffer, source_buffer_size);

    Metadata* metadata = STT_IntermediateDecodeWithMetadata(stream_ctx, 1);
    const std::string current = CandidateTranscriptToFormattedString(&metadata->transcripts[0]);
    STT_FreeMetadata((Metadata*)metadata);
    if (current != previous)
    {
      if (is_interactive) {
        // If this is outputting to a terminal, clear the screen so that
        // as the output changes the latest is always displayed.
        printf("\033[H\033[J%s", current.c_str());
        fflush(stdout);
      }
      else
      {
        // It looks like we're outputting to a file, so try to wait a little
        // for the transcription to stabilize. There's no API for this, so
        // instead use a heuristic of outputting n characters behind and hope
        // this is far enough back in time that they won't change.
        constexpr int wait_count = 10;
        const int previous_print_index = previous_print.length();
        const int start_print_index = std::max(0, (int)(current.length() - wait_count));
        if (start_print_index > previous_print_index) {
          const int char_count = start_print_index - previous_print_index;
          std::string new_output = current.substr(previous_print_index, char_count);
          printf("%s", new_output.c_str());
          fflush(stdout);
          previous_print += new_output;
        }
      }
      previous = current;
    }
  }

  result = 0;

finish:

  if (source_stream)
    pa_simple_free(source_stream);

  free((void*)(mic_buffer));

  return result;
}

int main(int argc, char** argv)
{
  if (!ProcessArgs(argc, argv))
  {
    return 1;
  }

  // Initialise STT
  ModelState* ctx;
  // sphinx-doc: c_ref_model_start
  int status = STT_CreateModel(model, &ctx);
  if (status != 0)
  {
    char* error = STT_ErrorCodeToErrorMessage(status);
    fprintf(stderr, "Could not create model: %s\n", error);
    free(error);
    return 1;
  }

  if (set_beamwidth)
  {
    status = STT_SetModelBeamWidth(ctx, beam_width);
    if (status != 0)
    {
      fprintf(stderr, "Could not set model beam width.\n");
      return 1;
    }
  }

  if (scorer)
  {
    status = STT_EnableExternalScorer(ctx, scorer);
    if (status != 0)
    {
      fprintf(stderr, "Could not enable external scorer.\n");
      return 1;
    }
    if (set_alphabeta)
    {
      status = STT_SetScorerAlphaBeta(ctx, lm_alpha, lm_beta);
      if (status != 0)
      {
        fprintf(stderr, "Error setting scorer alpha and beta.\n");
        return 1;
      }
    }
  }
  // sphinx-doc: c_ref_model_stop

  if (hot_words)
  {
    std::vector<std::string> hot_words_ = SplitStringOnDelim(hot_words, ",");
    for (std::string hot_word_ : hot_words_)
    {
      std::vector<std::string> pair_ = SplitStringOnDelim(hot_word_, ":");
      const char* word = (pair_[0]).c_str();
      // the strtof function will return 0 in case of non numeric characters
      // so, check the boost string before we turn it into a float
      bool boost_is_valid = (pair_[1].find_first_not_of("-.0123456789") == std::string::npos);
      float boost = strtof((pair_[1]).c_str(), 0);
      status = STT_AddHotWord(ctx, word, boost);
      if (status != 0 || !boost_is_valid)
      {
        fprintf(stderr, "Could not enable hot-word.\n");
        return 1;
      }
    }
  }

  if (strcmp(source, "file") == 0) {
    for (const std::string filename : filename_args) {
      TranscribeFiles(ctx, filename.c_str());
    }
  }
  else
  {
    TranscribeLiveSource(ctx, source);
  }

  STT_FreeModel(ctx);

  return 0;
}
