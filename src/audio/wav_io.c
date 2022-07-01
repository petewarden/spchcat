#include "wav_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_buffer.h"
#include "string_utils.h"
#include "trace.h"

static bool expect_data(const char* expected, int expected_size,
  FILE* file) {
  uint8_t* data = calloc(1, expected_size);
  fread(data, expected_size, 1, file);
  const bool result = (memcmp(data, expected, expected_size) == 0);
  free(data);
  return result;
}

static uint16_t fread_uint16(FILE* file) {
  uint16_t result;
  fread(&result, 2, 1, file);
  return result;
}

static void fwrite_uint16(const uint16_t value, FILE* file) {
  fwrite(&value, 2, 1, file);
}

static uint32_t fread_uint32(FILE* file) {
  uint32_t result;
  fread(&result, 4, 1, file);
  return result;
}

static void fwrite_uint32(const uint32_t value, FILE* file) {
  fwrite(&value, 4, 1, file);
}

bool wav_io_load(const char* filename, AudioBuffer** result) {
  *result = NULL;

  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Couldn't load file '%s'\n", filename);
    return false;
  }

  if (!expect_data("RIFF", 4, file)) {
    fprintf(stderr, "'RIFF' wasn't found in header of WAV file '%s'\n",
      filename);
    fclose(file);
    return false;
  }
  fread_uint32(file);  // file_size_minus_eight
  if (!expect_data("WAVE", 4, file)) {
    fprintf(stderr, "'WAVE' wasn't found in header of WAV file '%s'\n",
      filename);
    fclose(file);
    return false;
  }

  uint8_t found_chunk_id[4];
  fread(found_chunk_id, 4, 1, file);
  while (memcmp(found_chunk_id, "fmt ", 4) != 0) {
    const uint32_t chunk_size = fread_uint32(file);
    fseek(file, chunk_size, SEEK_CUR);
    fread(found_chunk_id, 4, 1, file);
  }
  const uint32_t format_chunk_size = fread_uint32(file);
  if ((format_chunk_size != 18) && (format_chunk_size != 16)) {
    fprintf(stderr,
      "Format chunk size was %d instead of 16 or 18 in WAV file '%s'\n",
      format_chunk_size, filename);
    return false;
  }
  const uint16_t format_type = fread_uint16(file);
  if (format_type != 1) {
    fprintf(stderr,
      "Format type was %d instead of 1 in WAV file '%s'\n",
      format_type, filename);
    return false;
  }
  const uint16_t channels = fread_uint16(file);
  const uint32_t sample_rate = fread_uint32(file);
  fread_uint32(file);  // bytes_per_second
  fread_uint16(file);  // bytes_per_sample
  const uint16_t bits_per_sample = fread_uint16(file);
  if (bits_per_sample != 16) {
    fprintf(stderr,
      "Bits per sample was %d instead of 16 in WAV file '%s'\n",
      bits_per_sample, filename);
    return false;
  }
  if (format_chunk_size == 18) {
    fseek(file, 2, SEEK_CUR);
  }

  fread(found_chunk_id, 4, 1, file);
  while (memcmp(found_chunk_id, "data", 4) != 0) {
    const uint32_t chunk_size = fread_uint32(file);
    fseek(file, chunk_size, SEEK_CUR);
  }

  const uint32_t chunk_size = fread_uint32(file);
  const int samples_per_channel = (chunk_size / channels) / 2;
  *result = audio_buffer_alloc(sample_rate, samples_per_channel, channels);
  fread((*result)->data, chunk_size, 1, file);

  fclose(file);
  return true;
}

bool wav_io_save(const char* filename, const AudioBuffer* buffer) {
  const int header_byte_count = 44;
  const int sample_bit_count = 16;
  const int sample_byte_count = 2;
  const int num_samples = buffer->samples_per_channel * buffer->channels;
  const int bytes_per_second =
    buffer->sample_rate * sample_byte_count * buffer->channels;
  const int bytes_per_frame = sample_byte_count * buffer->channels;
  const int data_byte_count = num_samples * sample_byte_count;
  const int file_size = header_byte_count + data_byte_count;

  FILE* file = fopen(filename, "wb");
  if (file == NULL) {
    fprintf(stderr, "Couldn't open file '%s' for saving.\n", filename);
    return false;
  }

  fwrite("RIFF", 4, 1, file);
  fwrite_uint32(file_size - 8, file);
  fwrite("WAVE", 4, 1, file);

  fwrite("fmt ", 4, 1, file);
  fwrite_uint32(16, file);
  fwrite_uint16(1, file);
  fwrite_uint16(buffer->channels, file);
  fwrite_uint32(buffer->sample_rate, file);
  fwrite_uint32(bytes_per_second, file);
  fwrite_uint16(bytes_per_frame, file);
  fwrite_uint16(sample_bit_count, file);

  fwrite("data", 4, 1, file);
  fwrite_uint32(data_byte_count, file);
  fwrite(buffer->data, data_byte_count, 1, file);

  fclose(file);

  return true;
}
