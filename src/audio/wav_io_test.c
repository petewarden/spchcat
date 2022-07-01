#include "acutest.h"

#include "wav_io.c"

#include "file_utils.h"

void test_expect_data() {
  const char* test_filename = "/tmp/test_expect_data";
  file_write(test_filename, "Foo", 3);

  FILE* file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  TEST_CHECK(expect_data("Foo", 3, file));
  fclose(file);

  file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  TEST_CHECK(!expect_data("Bar", 3, file));
  fclose(file);
}

void test_fread_uint16() {
  const char* test_filename = "/tmp/test_fread_uint16";
  uint16_t value = 0x3123;
  file_write(test_filename, (char*)(&value), 2);

  FILE* file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  TEST_INTEQ(0x3123, fread_uint16(file));
  fclose(file);
}

void test_fwrite_uint16() {
  const char* test_filename = "/tmp/test_fwrite_uint16";
  FILE* file = fopen(test_filename, "wb");
  TEST_CHECK(file != NULL);
  fwrite_uint16(0x3123, file);
  fclose(file);

  file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  uint16_t value;
  fread(&value, 2, 1, file);
  TEST_INTEQ(0x3123, value);
  fclose(file);
}

void test_fread_uint32() {
  const char* test_filename = "/tmp/test_fread_uint32";
  uint32_t value = 0x12345678;
  file_write(test_filename, (char*)(&value), 4);

  FILE* file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  TEST_INTEQ(0x12345678, fread_uint32(file));
  fclose(file);
}

void test_fwrite_uint32() {
  const char* test_filename = "/tmp/test_fwrite_uint16";
  FILE* file = fopen(test_filename, "wb");
  TEST_CHECK(file != NULL);
  fwrite_uint32(0x12345678, file);
  fclose(file);

  file = fopen(test_filename, "rb");
  TEST_CHECK(file != NULL);
  uint32_t value;
  fread(&value, 4, 1, file);
  TEST_INTEQ(0x12345678, value);
  fclose(file);
}

void test_wav_io_load() {
  const char* test_filename = "/tmp/test_wav_io_load.wav";
  unsigned char test_data[] = {
    'R', 'I', 'F', 'F',
    52, 0, 0, 0,
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  // Format chunk size.
    1, 0,  // Format type.
    2, 0,  // Channels.
    0x80, 0x3e, 0, 0,  // Sample rate.
    0x00, 0xfa, 0, 0,  // Bytes per second.
    4, 0,  // Bytes per frame (#channels).
    16, 0, // Bits per sample.
    'd', 'a', 't', 'a',
    16, 0, 0, 0,  // Data chunk size.
    23, 33,  // Sample #1, Channel #1.
    11, 77,  // Sample #1, Channel #2.
    101, 89,  // Sample #2, Channel #1.
    55, 91,  // Sample #2, Channel #2.
    117, 18,  // Sample #3, Channel #1.
    33, 212,  // Sample #3, Channel #2.
    169, 134,  // Sample #4, Channel #1.
    42, 121,  // Sample #4, Channel #2.
  };
  const size_t test_data_length = sizeof(test_data) / sizeof(test_data[0]);
  file_write(test_filename, (char*)(test_data), test_data_length);

  AudioBuffer* buffer = NULL;
  TEST_ASSERT(wav_io_load(test_filename, &buffer));
  TEST_ASSERT(buffer != NULL);
  TEST_INTEQ(2, buffer->channels);
  TEST_INTEQ(16000, buffer->sample_rate);
  TEST_INTEQ(4, buffer->samples_per_channel);
  TEST_CHECK(buffer->data != NULL);
  TEST_INTEQ(8471, buffer->data[0]);
  TEST_INTEQ(19723, buffer->data[1]);
  TEST_INTEQ(22885, buffer->data[2]);
  TEST_INTEQ(23351, buffer->data[3]);
  TEST_INTEQ(4725, buffer->data[4]);
  TEST_INTEQ(-11231, buffer->data[5]);
  TEST_INTEQ(-31063, buffer->data[6]);
  TEST_INTEQ(31018, buffer->data[7]);
  audio_buffer_free(buffer);
}

void test_wav_io_save() {
  const char* test_filename = "/tmp/test_wav_io_save.wav";

  AudioBuffer* buffer = audio_buffer_alloc(16000, 4, 2);
  buffer->data[0] = 8471;
  buffer->data[1] = 19723;
  buffer->data[2] = 22885;
  buffer->data[3] = 23351;
  buffer->data[4] = 4725;
  buffer->data[5] = -11231;
  buffer->data[6] = -31063;
  buffer->data[7] = 31018;

  TEST_CHECK(wav_io_save(test_filename, buffer));
  audio_buffer_free(buffer);

  unsigned char expected_data[] = {
    'R', 'I', 'F', 'F',  // #0
    52, 0, 0, 0,  // #4
    'W', 'A', 'V', 'E',  // #8
    'f', 'm', 't', ' ',  // #12
    16, 0, 0, 0,  // #16, Format chunk size.
    1, 0,  // #20, Format type.
    2, 0,  // #22, Channels.
    0x80, 0x3e, 0, 0,  // #24, Sample rate.
    0x00, 0xfa, 0, 0,  // #28, Bytes per second.
    4, 0,  // #32, Bytes per frame.
    16, 0,  // #34, Bits per sample.
    'd', 'a', 't', 'a',  // #36
    16, 0, 0, 0,  // #40, Data chunk size.
    23, 33,  // #44, Sample #1, Channel #1.
    11, 77,  // #46, Sample #1, Channel #2.
    101, 89,  // #48, Sample #2, Channel #1.
    55, 91,  // #50, Sample #2, Channel #2.
    117, 18,  // #52, Sample #3, Channel #1.
    33, 212,  // #54, Sample #3, Channel #2.
    169, 134,  // #56, Sample #4, Channel #1.
    42, 121,  // #58, Sample #4, Channel #2.
  };
  const size_t expected_data_length =
    sizeof(expected_data) / sizeof(expected_data[0]);

  FILE* file = fopen(test_filename, "rb");

  uint8_t* found_data = calloc(1, expected_data_length);
  fread(found_data, expected_data_length, 1, file);
  fclose(file);

  for (int i = 0; i < expected_data_length; ++i) {
    TEST_INTEQ(expected_data[i], found_data[i]);
    TEST_MSG("At position %d", i);
  }

  free(found_data);
}

void test_wav_io_save_listenable() {
  const char* test_filename = "/tmp/test_wav_io_save_listenable.wav";

  const int sample_rate = 16000;
  const int sample_count = sample_rate * 2;
  const int channels = 1;

  AudioBuffer* buffer = audio_buffer_alloc(sample_rate, sample_count, channels);

  for (int i = 0; i < sample_count; ++i) {
    const float phase = (float)(i) / 10.0f;
    buffer->data[i] = (int)(sinf(phase) * 32767);
  }

  TEST_CHECK(wav_io_save(test_filename, buffer));
  audio_buffer_free(buffer);
}

TEST_LIST = {
  {"expect_data", test_expect_data},
  {"fread_uint16", test_fread_uint16},
  {"fwrite_uint16", test_fwrite_uint16},
  {"fread_uint32", test_fread_uint32},
  {"fwrite_uint32", test_fwrite_uint32},
  {"wav_io_load", test_wav_io_load},
  {"wav_io_save", test_wav_io_save},
  {"wav_io_save_listenable", test_wav_io_save_listenable},
  {NULL, NULL},
};