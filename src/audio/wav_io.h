#ifndef INCLUDE_WAV_IO_H
#define INCLUDE_WAV_IO_H

#include <stdbool.h>

#include "audio_buffer.h"

bool wav_io_load(const char* filename, AudioBuffer** result);

bool wav_io_save(const char* filename, const AudioBuffer* buffer);

#endif  // INCLUDE_WAV_IO_H