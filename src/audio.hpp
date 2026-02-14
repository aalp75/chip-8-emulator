#ifndef CHIP8_AUDIO_H
#define CHIP8_ADUIO_H

#include <cstdint>
#include <SDL2/SDL_stdinc.h>
#include <cstring>

constexpr int SAMPLE_RATE = 44100;
constexpr int BEEP_FREQ   = 440;
constexpr int AMPLITUDE   = 8000;

struct AudioState {
    bool* audioPlaying;
    double* phase;
};

void audioCallback(void* userdata, Uint8* stream, int len);

#endif
