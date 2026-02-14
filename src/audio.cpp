#include "audio.hpp"

void audioCallback(void* userdata, Uint8* stream, int len) {
    auto* st = static_cast<AudioState*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / int(sizeof(int16_t));

    if (!(*st->audioPlaying)) {
        std::memset(stream, 0, len);
        return;
    }

    double& ph = *st->phase;
    const double step = 2.0 * M_PI * double(BEEP_FREQ) / double(SAMPLE_RATE);

    for (int i = 0; i < samples; i++) {
        buffer[i] = (std::sin(ph) > 0.0) ? AMPLITUDE : -AMPLITUDE;
        ph += step;
        if (ph >= 2.0 * M_PI) ph -= 2.0 * M_PI;
    }
}
