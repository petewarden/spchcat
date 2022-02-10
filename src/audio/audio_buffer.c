#include "audio_buffer.h"

#include "stdlib.h"

AudioBuffer* audio_buffer_alloc(int32_t sample_rate,
  int32_t samples_per_channel, int32_t channels) {
  AudioBuffer* result = calloc(1, sizeof(AudioBuffer));
  result->sample_rate = sample_rate;
  result->channels = channels;
  result->samples_per_channel = samples_per_channel;
  const size_t byte_count = (samples_per_channel * channels * sizeof(int16_t));
  result->data = calloc(1, byte_count);
  return result;
}

void audio_buffer_free(AudioBuffer* buffer) {
  if (buffer == NULL) {
    return;
  }
  free(buffer->data);
  free(buffer);
}
