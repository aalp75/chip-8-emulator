#include <SDL2/SDL.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring> // memset
#include <ctime>
#include <chrono>
#include <map>
#include <thread>
#include "font.hpp"
#include "chip8.hpp"
#include "audio.hpp"
#include "keyboard.hpp"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom>\n";
        return 1;
    }

    const char* rom = argv[1];

    Chip8 chip;

    if (!chip.loadRom(rom)) {
        return 0;
    }

    const int CHIP8_WIDTH  = 64;
    const int CHIP8_HEIGHT = 32;
    const int SCALE = 20;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chip-8 Monitor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        CHIP8_WIDTH * SCALE,
        CHIP8_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED
    );
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // -- Audio Part --
    bool audioPlaying = false;
    double phase = 0.0;

    SDL_AudioDeviceID audioDevice = 0;

    SDL_AudioSpec audioSpec{};
    audioSpec.freq = SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.samples = 512;

    AudioState audioState{ &audioPlaying, &phase };
    audioSpec.userdata = &audioState;
    audioSpec.callback = audioCallback;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
    if (!audioDevice) {
        std::cerr << "SDL_OpenAudioDevice Error: " << SDL_GetError() << "\n";   
    }
    
    SDL_PauseAudioDevice(audioDevice, 0);

    bool running = true;
    SDL_Event event;

    const std::chrono::duration<double> tick = std::chrono::duration<double>(1.0 / 60.0);
    auto nextTick = std::chrono::steady_clock::now() + tick;

    auto startTime = std::chrono::steady_clock::now();

    int countInstruction = 0;
    int countTick = 0;

    while (running) {

        chip.tick();
        nextTick += tick;
        countTick++;

        if (audioDevice) SDL_LockAudioDevice(audioDevice);
        audioPlaying = (chip.soundTimer > 0);
        if (audioDevice) SDL_UnlockAudioDevice(audioDevice);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    std::cout << "Quitting game!\n";
                    running = false;
                    break;
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int key = keyboardMapping(event.key.keysym.scancode);
                if (key != -1) {
                    chip.keyboard[key] = (event.type == SDL_KEYDOWN) ? true : false;
                }
            }
        }

        if (!running) break;

        // Run CHIP-8 instructions
        for (int i = 0; i < 10; i++) {
            chip.cycle();
            countInstruction++;
        }

        // Draw pixels if frame buffer has been updated
        if (chip.updateDisplay()) {
            for (int y = 0; y < CHIP8_HEIGHT; y++) {
                for (int x = 0; x < CHIP8_WIDTH; x++) {
                    if (chip.framebuffer[y * CHIP8_WIDTH + x]) { // black pixel
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    }
                    else { // white pixel
                        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
                    }
                    SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
            chip.displayUpdated();
        }

        SDL_RenderPresent(renderer);

        auto time = std::chrono::steady_clock::now();
        if (time < nextTick) {
            std::this_thread::sleep_for(nextTick - time);
        }
    }

    if (audioDevice) {
        SDL_LockAudioDevice(audioDevice);
        audioPlaying = false;
        SDL_UnlockAudioDevice(audioDevice);
        SDL_CloseAudioDevice(audioDevice);
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    auto endTime = std::chrono::steady_clock::now();

    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Total time played: " << elapsedSeconds << " seconds.\n";
    std::cout << "CPU Frequency: " << (0.0 + countInstruction) / elapsedSeconds << " IPS\n";
    std::cout << "Frame Timer Frequency: " << (0.0 + countTick) / elapsedSeconds << "Hz \n";

    return 0;
}