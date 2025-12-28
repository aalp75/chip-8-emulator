#include <SDL2/SDL.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring> // memset


class Chip8 {

public: // set all to public for now

    // constant
    static constexpr int START_ADDRESS = 0x200; // (512);
    static constexpr int FRAMEBUFFER_WIDTH  = 64;
    static constexpr int FRAMEBUFER_HEIGHT = 32;

    static const unsigned int FONTSET_SIZE = 80;

    static const unsigned int FONTSET_START_ADDRESS = 0x50;

    uint8_t fontset[FONTSET_SIZE] = {
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

    uint8_t registers[16];   // 16 8 bit registers
    uint8_t memory[4096];    // 4096b ytes of RAM

    uint16_t stack[16];      // Stack

    uint16_t I;              // Pointer address
    uint16_t PC;             // Program counter
    uint8_t SP;              // Stack pointer
    uint8_t delay_reg;       // Delay timer
    uint8_t sound_reg;       // Sound timer

    uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFER_HEIGHT]; // Display Framebuffer Top-Left to Bottom-Right

public:

    Chip8() : PC(START_ADDRESS) {
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
            return;
        }

        if ((opcode & 0xF000) == 0x2000) { // CALL addr
            uint16_t address = opcode & 0x0FFF;
            stack[SP] = PC;
            SP++;
            PC = address;
            return;
        }

        if ((opcode & 0xF000) == 0xA000) { // LD I, addr
            I = opcode & 0x0FFF;
            return;
        }

        if ((opcode & 0xF000) == 0x6000) { // LD, Vx, byte
            uint8_t Vx = (opcode & 0xF00u) >> 8;
            uint8_t byte = opcode & 0x00FF;
            registers[Vx] = byte;
            return;
        }

        switch (opcode) {

        case 0x00E0: // CLS (clear the display)
            std::memset(framebuffer, 0, sizeof(framebuffer));
            break;

        case 0x00EE: // RET (return from a subroutine)
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
        std::cout << "Execute instruction: ";
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
            for (long i = 0; i < size; i += 2) {
                memory[START_ADDRESS + i] = buffer[i];
                memory[START_ADDRESS + i + 1] = buffer[i + 1];

                uint16_t opcode = (memory[START_ADDRESS + i] << 8) | memory[START_ADDRESS + i + 1];

                std::cout
                    << "opcode[" << (i / 2) << "] = 0x"
                    << std::hex
                    << std::setw(4)
                    << std::setfill('0')
                    << opcode
                    << std::dec
                    << '\n';
            }

            // Free the buffer
            delete[] buffer;
        }
        return true;
    }
};


int main() {


    Chip8 chip;

    chip.loadRom("roms/IBM Logo.ch8");

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
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Run some CHIP-8 instructions
        for (int i = 0; i < 10; i++) {
            std::cout << "cycle " << i << '\n';
            chip.cycle();
        }

        // Clear screen (black)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw pixels
        for (int y = 0; y < CHIP8_HEIGHT; y++) {
            for (int x = 0; x < CHIP8_WIDTH; x++) {
                if (chip.framebuffer[y * CHIP8_WIDTH + x]) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE, SCALE };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // sleep time in milliseconds
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}