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
#include "log.cpp"
#include "font.h"

class Chip8 {

public: // set all to public for now

    // constant
    static constexpr uint16_t PROGRAM_START_ADDRESS = 0x200; // (512);
    static constexpr uint16_t FONTSET_START_ADDRESS = 0x50; // (80)

    static constexpr uint16_t FRAMEBUFFER_WIDTH  = 64;
    static constexpr uint16_t FRAMEBUFER_HEIGHT = 32;

    uint8_t registers[16];  // 16 1 byte registers
    uint8_t memory[4096];   // 4096 bytes of RAM

    uint16_t stack[16];     // Stack (16 bits because it's memory address)
                            // Stack can also be implemented on the first bytes RAM

    uint16_t I;             // Pointer address
    uint16_t PC;            // Program counter (16 bits because 8 bits is too small)
    uint8_t SP;             // Stack pointer

    uint8_t delayTimer;     // Delay timer
    uint8_t soundTimer;     // Sound timer

    std::mt19937 rng;       // Random generator

    // it should be 60 Hz (~60 fps)
    uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFER_HEIGHT]; // Display Framebuffer Top-Left to Bottom-Right

    bool keyboard[16];      // Keyboard

    bool waitingInput;
    uint8_t Rx;

    bool updateDisplayFlag = false; // flag to update display
    int pressedKey = -1; // used for Fx0A

public:

    Chip8()
    : PC(PROGRAM_START_ADDRESS)
    , rng(42)
    , I(0)
    , SP(0)
    , delayTimer(0)
    , soundTimer(0)
    , updateDisplayFlag(false)
    {
        std::memset(registers, 0, sizeof(registers));
        std::memset(memory, 0, sizeof(memory));
        std::memset(stack, 0, sizeof(stack));
        std::memset(framebuffer, 0, sizeof(framebuffer));
        std::memset(keyboard, 0, sizeof(keyboard));

        // Load font at the beggining of the memory
        // between FONTSET_START_ADDRESS and PROGRAM_START_ADDRESS
        for (unsigned int i = 0; i < chip8Font::FONTSET_SIZE; i++) {
            memory[FONTSET_START_ADDRESS + i] = chip8Font::fontset[i];
        }
    }

    std::ostream& printOpCode(std::ostream& os, uint16_t opcode) {
        os << std::hex << std::setw(4) << std::setfill('0') << opcode << std::dec;
        return os;
    }

    // fetch opcode on 2 bytes (e.g. 0x00E0)
    uint16_t fetchOpCode() const {
        uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
        return opcode;
    }

    /* 
    * Chip-8 Set of Instructions
    *
    * http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.3
    */
    void execute(uint16_t opcode) {

        if (opcode == 0x00E0) { // CLS (clear the display)
            logTimePrefix(); 
            std::cout << "CLS\n";
            std::memset(framebuffer, 0, sizeof(framebuffer));
            updateDisplayFlag = true;
            return;
        }

        if (opcode == 0x00EE) { // RET (return from a subroutine)
            logTimePrefix();
            std::cout << "RET\n";
            SP--;
            PC = stack[SP];
            return;
        }

        if ((opcode & 0xF000) == 0x1000) { // 1nnn - JP addr
            uint16_t address = opcode & 0x0FFF;
            PC = address;
            logTimePrefix();
            std::cout << "JP " << PC << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0x2000) { // 2nnn - CALL addr
            uint16_t address = opcode & 0x0FFF;
            stack[SP] = PC;
            SP++;
            PC = address;
            logTimePrefix();
            std::cout << "CALL " << address << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0x3000) { // 3xkk - SE Vx, byte
            // Skip next instruction if Vx = kk
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t byte = opcode & 0x00FF;

            logTimePrefix();
            std::cout << "SE V" << unsigned(x) << ", " << unsigned(byte) << '\n';

            if (registers[x] == byte) {
                PC += 2;
            }
            return;
        }

        if ((opcode & 0xF000) == 0x4000) { // 4xkk - SNE Vx, byte 
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t byte = opcode & 0x00FF;

            if (registers[x] != byte) {
                PC += 2;
            }

            logTimePrefix();
            std::cout << "SNE V" << unsigned(x) << ", " << unsigned(byte) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x5000) { // 5xy0 - SE Vx, Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode &0x00F0) >> 4;

            if (registers[x] == registers[y]) {
                PC += 2;
            }
            logTimePrefix();
            std::cout << "SE V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0x6000) { // 6xkk - LD Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t byte = opcode & 0x00FF;
            logTimePrefix();
            std::cout << "LD " << unsigned(x) << ", " << unsigned(byte) << '\n';
            registers[x] = byte;
            return;
        }

        if ((opcode & 0xF000) == 0x7000) { // 7xkk - ADD Vx, byte

            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t add = opcode & 0x00FF;

            logTimePrefix();
            std::cout << "ADD " << unsigned(Vx) << ", " << unsigned(add) << '\n';

            registers[Vx] += add;
            return;
        }

        if ((opcode & 0xF00F) == 0x8000) { // 8xy0 - LD Vx, Vy

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            registers[x] = registers[y];
            logTimePrefix();
            std::cout << "LD V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x8001) { // 8xy1 - OR Vx, Vy

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            registers[x] |= registers[y];
            logTimePrefix();
            std::cout << "OR V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x8002) { // 8xy2 - AND Vx, Vy

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            registers[x] &= registers[y];
            logTimePrefix();
            std::cout << "AND V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x8003) { // 8xy3 - XOR Vx, Vy

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            registers[x] ^= registers[y];
            logTimePrefix();
            std::cout << "XOR V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x8004) { // 8xy4 - ADD Vx, Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            uint16_t sum = uint16_t(registers[x]) + uint16_t(registers[y]);
            registers[0xF] = (sum > 0xFF) ? 1 : 0; // greater than 255
            registers[x] = uint8_t(sum & 0xFF);

            logTimePrefix();
            std::cout << "ADD V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
            }

        if ((opcode & 0xF00F) == 0x8005) { // 8xy5 - SUB Vx, Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            registers[0xF] = 0;
            if (registers[x] >= registers[y]) {
                registers[0xF] = 1;
            }
            registers[x] -= registers[y];

            logTimePrefix();
            std::cout << "SUB V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF00F) == 0x8006) { // 8xy6 - SHR Vx {, Vy}
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            registers[0xF] = 0;
            if ((registers[x] & 1) == 1) {
                registers[0xF] = 1;
            }

            registers[x] = registers[x] >> 1;

            logTimePrefix();
            std::cout << "SHR V" << unsigned(x) << " {, V" << unsigned(y) << "}\n";
            return;
        }

        if ((opcode & 0xF00F) == 0x8007) { // 8xy7 - SUBN Vx, Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            registers[0xF] = 0;
            if (registers[y] >= registers[x]) {
                registers[0xF] = 1;
            }

            registers[x] = registers[y] - registers[x];
            logTimePrefix();
            std::cout << "SUBN V" << unsigned(x) << ", V" << unsigned(y) << "}\n";
            return;
        }

        if ((opcode & 0xF00F) == 0x800E) { // 8xyE - SHL Vx {, Vy}
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            registers[0xF] = 0;
            if ((registers[x] & 128) > 0) { // most significant bit set to 1
                registers[0xF] = 1;
            }

            registers[x] = registers[x] << 1;
            logTimePrefix();
            std::cout << "SHL V" << unsigned(x) << " {, V" << unsigned(y) << "}\n";
            return;
        }

        if ((opcode & 0xF00F) == 0x9000) { // 9xy0 - SNE Vx, Vy
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode &0x00F0) >> 4;

            if (registers[x] != registers[y]) {
                PC += 2;
            }
            logTimePrefix();
            std::cout << "SNE V" << unsigned(x) << ", V" << unsigned(y) << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0xA000) { // Annn - LD I, addr
            I = opcode & 0x0FFF;
            logTimePrefix();
            std::cout << "LD I " << I << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0xC000) { // Cxkk - RND Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t byte = opcode & 0x00FF;

            logTimePrefix(); 
            std::cout << "RND V" << unsigned(x) << ", " << byte << '\n';

            registers[x] = (rng() % 256) & byte;
            return;
        }

        if ((opcode & 0xF000) == 0xD000) { // Dxyn - DRW Vx, Vy nibble
            // display the n-byte sprite starting at memory location I at (Vx, Vy)
            // set registers F = collision

            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t Vy = (opcode & 0x00F0) >> 4;
            uint8_t height = opcode & 0x000F;

            logTimePrefix();
            std::cout << "DRW " << unsigned(Vx) << ", " << unsigned(Vy) << ", " << unsigned(height) << '\n';

            uint8_t xPos = registers[Vx];
            uint8_t yPos = registers[Vy];

            registers[0xF] = 0;

            for (unsigned int row = 0; row < height; row++) {
                uint8_t sprite = memory[I + row];
                for (unsigned int bit = 0; bit < 8; bit++) {
                    if (sprite & (0x80 >> bit)) {
                        int px = (xPos + bit) % 64;
                        int py = (yPos + row) % 32;
                        int idx = py * 64 + px;

                        if (framebuffer[idx]) registers[0xF] = 1;
                        framebuffer[idx] ^= 1;
                    }
                }
            }
            updateDisplayFlag = true;
            return;
        }

        if ((opcode & 0xF0FF) == 0xE09E) { // Ex9E - SKP Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            if (keyboard[registers[x]]) {
                PC += 2;
            }
            logTimePrefix();
            std::cout << "SKP V" << unsigned(x) << '\n';
            return;            
        }

        if ((opcode & 0xF0FF) == 0xE0A1) { // ExA1 - SKNP Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            if (!keyboard[registers[x]]) {
                PC += 2;
            }
            logTimePrefix();
            std::cout << "SKNP V" << unsigned(x) << '\n';
            return;            
        }

        if ((opcode & 0xF0FF) == 0xF007) { // Fx07 - LD Vx, DT
            uint8_t x = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD " << unsigned(x) << ", " << "DT\n";
            registers[x] = delayTimer;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF00A) { // Fx0A - LD Vx, K
            uint8_t x = (opcode & 0x0F00) >> 8;

            if (pressedKey == -1) {
                for (int key = 0; key < 16; key++) {
                    if (keyboard[key]) {
                        pressedKey = key;
                        break;
                    }
                }
            }

            if (pressedKey != -1 && !keyboard[pressedKey]) {
                registers[x] = pressedKey;

                logTimePrefix();
                std::cout << "LD V" << unsigned(x) << ", " << pressedKey << '\n';

                pressedKey = -1;
                return;
            }

            PC -= 2; // Decrement PC to repeat the instruction
            return;
        }

        if ((opcode & 0xF0FF) == 0xF015) { // Fx15 - LD DT, Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            delayTimer = registers[x];
            logTimePrefix();
            std::cout << "LD DT, V" << unsigned(x) << '\n';
            return;
        }

        if ((opcode & 0xF0FF) == 0xF018) { // Fx18 - LD ST, Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            soundTimer = registers[x];
            logTimePrefix();
            std::cout << "LD ST, V" << unsigned(x) << '\n';
            return;
        }

        if ((opcode & 0xF0FF) == 0xF01E) { // Fx1E - ADD I, Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            I += registers[x];
            logTimePrefix();
            std::cout << "ADD I, V" << unsigned(x) << '\n';
            return;
        }

        if ((opcode & 0xF0FF) == 0xF029) { // Fx29: LD F, Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD F, " << unsigned(x) << '\n';
            I = FONTSET_START_ADDRESS + registers[x] * 5;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF033) { // Fx33 - LD B, Vx

            uint8_t x = (opcode & 0x0F00) >> 8;

            logTimePrefix();
            std::cout << "LD B, " << unsigned(x) << '\n';

            memory[I] = registers[x] / 100;
            memory[I + 1] = (registers[x] / 10) % 10;
            memory[I + 2] = registers[x] % 10;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF055) { // Fx55 - LD [I], Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD [I], V" << unsigned(x) << '\n';
            for (unsigned int i = 0; i <= x; i++) {
                memory[I + i] = registers[i];
            }       
            return;
        }

        if ((opcode & 0xF0FF) == 0xF065) { // Fx65 - LD Vx [I]
            uint8_t x = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD " << unsigned(x) << ", [I]\n";
            for (unsigned int i = 0; i <= x; i++) {
                registers[i] = memory[I + i];
            }       
            return;
        }

        std::cout << "Unimplemented opcode: 0x" << std::hex << std::setw(4)
                  << std::setfill('0') << opcode << std::dec << '\n';
        std::exit(1);
    }

    void tick() {
        if (delayTimer > 0) delayTimer--;
        if (soundTimer > 0) soundTimer--;
    }

    void cycle() {
        uint16_t opcode = fetchOpCode();
        logTimePrefix();
        std::cout << "Execute instruction ";
        printOpCode(std::cout, opcode);
        std::cout << '\n';
        PC += 2;
        execute(opcode);
    }

    // load road into memory
    bool loadRom(const char* filename) {

        // Open the file as a stream of binary and move the file pointer to the end
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (file.is_open())
        {
            // Get size of file and allocate a buffer to hold the contents
            std::streampos size = file.tellg();
            char* buffer = new char[size];

            // Go back to the beginning of the file and fill the buffer
            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

            std::cout << "rom size = " << size << '\n';

            // Load the ROM contents into the Chip8's memory, starting at 0x200
            for (long i = 0; i < size; i++) {
                memory[PROGRAM_START_ADDRESS + i] = buffer[i];
            }

            // Free the buffer
            delete[] buffer;
        }
        return true;
    }

    bool updateDisplay() const {
        return updateDisplayFlag;
    }

    void displayUpdated() {
        updateDisplayFlag = false;
    }

    void printRegisters() const {
        std::cout << "Registers:\n";
        for (int i = 0; i < 16; i++) {
            std::cout
                << "V" << std::setw(2) << i
                << " = ["
                << std::setw(3) << std::right << unsigned(registers[i])
                << "]\n";
        }
    }
};

// It's better to use scancode than keycode 
// because it's not sensible to the keyboard region (e.g. QWERTY or AZERTY)

int keyboardMapping(SDL_Scancode scancode) {
    switch (scancode) {
        case SDL_SCANCODE_1: return 0x1;
        case SDL_SCANCODE_2: return 0x2;
        case SDL_SCANCODE_3: return 0x3;
        case SDL_SCANCODE_4: return 0xC;

        case SDL_SCANCODE_Q: return 0x4;
        case SDL_SCANCODE_W: return 0x5;
        case SDL_SCANCODE_E: return 0x6;
        case SDL_SCANCODE_R: return 0xD;

        case SDL_SCANCODE_A: return 0x7;
        case SDL_SCANCODE_S: return 0x8;
        case SDL_SCANCODE_D: return 0x9;
        case SDL_SCANCODE_F: return 0xE;

        case SDL_SCANCODE_Z: return 0xA;
        case SDL_SCANCODE_X: return 0x0;
        case SDL_SCANCODE_C: return 0xB;
        case SDL_SCANCODE_V: return 0xF;

        default: return -1;
    }
}


int main() {

    Chip8 chip;

    //chip.loadRom("roms/IBM Logo.ch8");
    //chip.loadRom("roms/pong.ch8");
    //chip.loadRom("roms/6-keypad.ch8"); // TODO: Fix the 3. (Halting not working)
    //chip.loadRom("roms/8-scrolling.ch8");
    //chip.loadRom("roms/bc_test.ch8");
    chip.loadRom("roms/br8kout.ch8");

    const int CHIP8_WIDTH  = 64;
    const int CHIP8_HEIGHT = 32;
    const int SCALE = 20;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED
    );

    bool running = true;
    SDL_Event event;

    using clock = std::chrono::steady_clock;

    const std::chrono::duration<double> tick = std::chrono::duration<double>(1.0 / 60.0);
    auto nextTick = clock::now() + tick;
    while (running) {

        chip.tick();
        nextTick += tick;

        //logTimePrefix();
        //std::cout << "running...\n";
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    std::cout << "ESCAPE\n";
                    running = false;
                    break;
                }
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
        }

        // Draw pixels if update
        if (chip.updateDisplay()) {
        //if (true) {
            for (int y = 0; y < CHIP8_HEIGHT; y++) {
                for (int x = 0; x < CHIP8_WIDTH; x++) {
                    if (chip.framebuffer[y * CHIP8_WIDTH + x]) { // black
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                    else { // white
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
            chip.displayUpdated();
        }

        SDL_RenderPresent(renderer);
        //SDL_Delay(16); // sleep time in milliseconds (16 ~ 1/60 seconds (60 Hz))

        auto time = clock::now();
        if (time < nextTick) {
            std::this_thread::sleep_for(nextTick - time);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}