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

class Chip8 {

public: // set all to public for now

    // constant
    static constexpr uint16_t PROGRAM_START_ADDRESS = 0x200; // (512);
    static constexpr uint16_t FONTSET_START_ADDRESS = 0x50; // (80)

    static constexpr uint16_t FRAMEBUFFER_WIDTH  = 64;
    static constexpr uint16_t FRAMEBUFFER_HEIGHT = 32;

    static constexpr uint16_t MEMORY_SIZE = 4096;

    uint8_t registers[16];  // 16 1 byte registers
    uint8_t memory[MEMORY_SIZE];   // 4096 bytes of RAM

    uint16_t stack[16];     // Stack (16 bits because it's memory address)
                            // Stack can also be implemented on the first bytes RAM

    uint16_t I;             // Pointer address
    uint16_t PC;            // Program counter (16 bits because 8 bits is too small)
    uint8_t SP;             // Stack pointer

    uint8_t delayTimer;     // Delay timer
    uint8_t soundTimer;     // Sound timer

    std::mt19937 rng;       // Random generator

    // it should be 60 Hz (~60 fps)
    uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT]; // Display Framebuffer Top-Left to Bottom-Right

    bool keyboard[16];      // Keyboard

    bool waitingInput;
    uint8_t Rx;

    bool updateDisplayFlag = false; // flag to update display
    int pressedKey = -1; // used for Fx0A

public:

    Chip8();

    std::ostream& printOpCode(std::ostream& os, uint16_t opcode);
    uint16_t fetchOpCode() const;
    void execute(uint16_t opcode); // Chip-8 Set of Instructions
    void tick();
    void cycle();
    bool loadRom(const char* filename); // load road into memory
    bool updateDisplay() const;
    void displayUpdated();
    void printRegisters() const;
};
