#ifndef INCLUDE_AUDIO_BUFFER_H
#define INCLUDE_AUDIO_BUFFER_H

#include <stdint.h>

typedef struct AudioBufferStruct {
  int32_t sample_rate;
  int32_t samples_per_channel;
  int32_t channels;
  // Convention is that samples are stored interleaved by channel, so 
  // |C0|C1|C0|C1|...
  int16_t* data;
} AudioBuffer;

AudioBuffer* audio_buffer_alloc(int32_t sample_rate,
  int32_t samples_per_channel, int32_t channels);
void audio_buffer_free(AudioBuffer* buffer);

#endif  // INCLUDE_AUDIO_BUFFER_H