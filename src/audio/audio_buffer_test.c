#include "acutest.h"

#include "audio_buffer.c"

static void test_audio_buffer_alloc() {
  AudioBuffer* buffer = audio_buffer_alloc(16000, 320, 2);
  TEST_CHECK(buffer != NULL);
  TEST_INTEQ(16000, buffer->sample_rate);
  TEST_INTEQ(320, buffer->samples_per_channel);
  TEST_INTEQ(2, buffer->channels);
  TEST_CHECK(buffer->data != NULL);
  audio_buffer_free(buffer);
}

TEST_LIST = {
  {"audio_buffer_alloc", test_audio_buffer_alloc},
  {NULL, NULL},
};