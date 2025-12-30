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
#include "log.cpp"

class Chip8 {

public: // set all to public for now

    // constant
    static constexpr unsigned int START_ADDRESS = 0x200; // (512);
    static constexpr unsigned int FONTSET_START_ADDRESS = 0x50; // (80)

    static constexpr unsigned int FRAMEBUFFER_WIDTH  = 64;
    static constexpr unsigned int FRAMEBUFER_HEIGHT = 32;

    static constexpr unsigned int FONTSET_SIZE = 80;

    static constexpr uint8_t fontset[FONTSET_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    uint8_t registers[16];  // 16 1 byte registers
    uint8_t memory[4096];   // 4096 bytes of RAM

    uint16_t stack[16];     // Stack (16 bits because it's memory address)

    uint16_t I;             // Pointer address
    uint16_t PC;            // Program counter (16 bits because 8 bits is too small)
    uint8_t SP;             // Stack pointer

    uint8_t delayTimer;     // Delay timer
    uint8_t soundTimer;     // Sound timer

    std::mt19937 rng;       // Random generator

    uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFER_HEIGHT]; // Display Framebuffer Top-Left to Bottom-Right

public:

    Chip8() : PC(START_ADDRESS), rng(42) {
        for (unsigned int i = 0; i < FONTSET_SIZE; i++) {
            memory[FONTSET_START_ADDRESS + i] = fontset[i];
        }
    }

    std::ostream& printOpCode(std::ostream& os, uint16_t opcode) {
        os << std::hex << std::setw(4) << std::setfill('0') << opcode << std::dec;
        return os;
    }

    uint16_t fetchOpCode() const {
        uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
        return opcode;
    }

    void execute(uint16_t opcode) {

        if ((opcode & 0xF000) == 0x1000) { // JP addr (jump to location nnn)
            uint16_t address = opcode & 0x0FFF;
            PC = address;
            logTimePrefix();
            std::cout << "JP " << PC << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0x2000) { // CALL addr
            uint16_t address = opcode & 0x0FFF;
            stack[SP] = PC;
            SP++;
            PC = address;
            logTimePrefix();
            std::cout << "CALL " << address << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0xA000) { // LD I, addr
            I = opcode & 0x0FFF;
            logTimePrefix();
            std::cout << "LD I " << I << '\n';
            return;
        }

        if ((opcode & 0xF000) == 0x6000) { // LD Vx, byte
            uint8_t Vx = (opcode & 0xF00u) >> 8;
            uint8_t byte = opcode & 0x00FF;
            logTimePrefix();
            std::cout << "LD " << unsigned(Vx) << ", " << unsigned(byte) << '\n';
            registers[Vx] = byte;
            return;
        }

        if ((opcode & 0xF000) == 0xD000) { // DRW Vx, Vy nibble
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
            return;
        }

        if ((opcode & 0xF000) == 0x7000) { // ADD Vx, byte

            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t add = opcode & 0x00FF;

            logTimePrefix();
            std::cout << "ADD " << unsigned(Vx) << ", " << unsigned(add) << '\n';

            registers[Vx] += add;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF033) { // LD B, Vx

            uint8_t Vx = (opcode & 0x0F00) >> 8;

            logTimePrefix();
            std::cout << "LD B, " << unsigned(Vx) << '\n';

            memory[I] = registers[Vx] / 100;
            memory[I + 1] = (registers[Vx] / 10) % 10;
            memory[I + 2] = registers[Vx] % 10;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF065) { // LD Vx [I]
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD " << unsigned(Vx) << ", [I]\n";
            for (unsigned int i = 0; i <= Vx; i++) {
                registers[i] = memory[I + i];
            }       
            return;
        }

        if ((opcode & 0xF0FF) == 0xF029) { // Fx29: LD F, Vx
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD F, " << unsigned(Vx) << '\n';
            I = FONTSET_START_ADDRESS + registers[Vx] * 5;
            return;
        }

        if ((opcode & 0xF0FF) == 0xF015) { // Fx15: LD DT, Vx
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            delayTimer = registers[Vx];
            logTimePrefix();
            std::cout << "LD DT, " << unsigned(Vx) << '\n';
            return;
        }

        if ((opcode & 0xF0FF) == 0xF007) { // Fx07 - LD Vx, DT
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            logTimePrefix();
            std::cout << "LD " << unsigned(Vx) << ", " << "DT\n";
            registers[Vx] = delayTimer;
            return;
        }

        if ((opcode & 0xF000) == 0x3000) { // 3xkk - SE Vx, byte
            // Skip next instruction if Vx != kk
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t byte = opcode & 0x00FF;

            logTimePrefix();
            std::cout << "SE V" << unsigned(x) << ", " << unsigned(byte) << '\n';

            if (registers[x] != byte) {
                PC += 2;
            }
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

        if ((opcode & 0xF0FF) == 0xE0A1) { // ExA1 - SKNP Vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            // TODO: set up the keyboard interaction
            
        }


        switch (opcode) {

        case 0x00E0: // CLS (clear the display)
            logTimePrefix();
            std::cout << "CLS\n";
            std::memset(framebuffer, 0, sizeof(framebuffer));
            break;

        case 0x00EE: // RET (return from a subroutine)
            logTimePrefix();
            std::cout << "RET\n";
            SP--;
            PC = stack[SP];
            break;

        default:
            std::cout << "Unimplemented opcode: 0x" << std::hex << std::setw(4)
                      << std::setfill('0') << opcode << std::dec << '\n';

            std::exit(1);
        }
    }

    void cycle() {
        uint16_t opcode = fetchOpCode();
        //std::cout << "Execute instruction: ";
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
                memory[START_ADDRESS + i] = buffer[i];
            }

            // Free the buffer
            delete[] buffer;
        }
        return true;
    }
};


int main() {


    Chip8 chip;

    //chip.loadRom("roms/IBM Logo.ch8");
    chip.loadRom("roms/pong.ch8");

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

    while (running) {
        logTimePrefix();
        std::cout << "running...\n";
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Run CHIP-8 instructions
        for (int i = 0; i < 1; i++) {
            std::cout << "cycle " << i << '\n';
            chip.cycle();
        }

        // Clear screen (black)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw pixels
        for (int y = 0; y < CHIP8_HEIGHT; y++) {
            for (int x = 0; x < CHIP8_WIDTH; x++) {
                if (chip.framebuffer[y * CHIP8_WIDTH + x]) { // black
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                    SDL_RenderFillRect(renderer, &pixel);
                }
                else { // white
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                    SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(10); // sleep time in milliseconds
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}