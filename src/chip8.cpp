#include "chip8.hpp"
#include "log.hpp"

Chip8::Chip8()
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

std::ostream& Chip8::printOpCode(std::ostream& os, uint16_t opcode) {
    os << std::hex << std::setw(4) << std::setfill('0') << opcode << std::dec;
    return os;
}

// fetch opcode on 2 bytes (e.g. 0x00E0)
uint16_t Chip8::fetchOpCode() const {
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    return opcode;
}

/* 
* Chip-8 Set of Instructions
*
* http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.3
*/
void Chip8::execute(uint16_t opcode) {

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
        if (SP == 0) {
            std::cout << "Stack pointer becomes negative\n";
            std::exit(1);
        }
        SP--;
        PC = stack[SP];
        return;
    }

    // Instruction ignored on modern chip-8
    if ((opcode & 0xF000) == 0x0000) { // SYS addr
        uint16_t address = opcode & 0x0FFF;
        logTimePrefix();
        std::cout << "SYS " << address << '\n';
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
        if (SP >= 16) {
            std::cout << "Stack overflow\n";
            std::exit(1);
        }
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
        std::cout << "LD V" << unsigned(x) << ", " << unsigned(byte) << '\n';
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

    if ((opcode & 0xF000) == 0xB000) { // Bnnn - JP, V0, addr

        uint16_t address = opcode & 0x0FFF;
        PC = registers[0] + address;
        std::cout << "JP, V0, " << address << '\n';
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
        uint8_t key = registers[x] & 0x0F;
        if (keyboard[key]) {
            PC += 2;
        }
        logTimePrefix();
        std::cout << "SKP V" << unsigned(x) << '\n';
        return;            
    }

    if ((opcode & 0xF0FF) == 0xE0A1) { // ExA1 - SKNP Vx
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t key = registers[x] & 0x0F;
        if (!keyboard[key]) {
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

void Chip8::tick() {
    if (delayTimer > 0) delayTimer--;
    if (soundTimer > 0) soundTimer--;
}

void Chip8::cycle() {
    uint16_t opcode = fetchOpCode();
    logTimePrefix();
    std::cout << "Execute instruction ";
    printOpCode(std::cout, opcode);
    std::cout << '\n';
    PC += 2;
    execute(opcode);
}

    // load road into memory
bool Chip8::loadRom(const char* filename) {

    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open ROM:" << filename << "\n";
        return false;
    }

    std::streampos romSize = file.tellg();
    int maxRomSize = MEMORY_SIZE - PROGRAM_START_ADDRESS;
    if (romSize > maxRomSize) {
        std::cout << "Rom is too large: " << romSize << " bytes (max is "
                    << maxRomSize << ")\n";         
        return false;
    }

    char* buffer = new char[romSize];

    file.seekg(0, std::ios::beg);
    file.read(buffer, romSize);
    file.close();

    for (long i = 0; i < romSize; i++) {
        memory[PROGRAM_START_ADDRESS + i] = buffer[i];
    }

    delete[] buffer;
    return true;
}

bool Chip8::updateDisplay() const {
    return updateDisplayFlag;
}

void Chip8::displayUpdated() {
    updateDisplayFlag = false;
}

void Chip8::printRegisters() const {
    std::cout << "Registers:\n";
    for (int i = 0; i < 16; i++) {
        std::cout
            << "V" << std::setw(2) << i
            << " = ["
            << std::setw(3) << std::right << unsigned(registers[i])
            << "]\n";
    }
}
