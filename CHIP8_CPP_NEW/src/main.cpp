#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stack>
#include <map>
#include <SDL2/SDL.h>

class Register {
public:
    unsigned int value;
    int bits;

    Register(int bits) {
        this->bits = bits;
        value = 0;
    }
    Register() {
        this->bits = 8;
        value = 0;
    }

    int checkCarry() {
        unsigned int maxValue = (1 << bits) - 1;
        if (value > maxValue) {
            value &= maxValue; // Keep only the lower bits
            return 1;
        }
        return 0;
    }

    int checkBorrow() {
        if (value > ((1 << bits) - 1)) {
            // value is unsigned, so no need for abs()
            value = (1 << bits) - value;  // Handle unsigned "negativity" by wrapping it
            return 0;
        }
        return 1;
    }

    std::string readValue() {
        std::stringstream ss;
        ss << std::hex << value;
        return ss.str();
    }

    void setValue(unsigned int val) {
        value = val;
    }
};

class DelayTimer {
public:
    int timer;

    DelayTimer() {
        timer = 0;
    }

    void countDown() {
        if (timer > 0) {
            timer--;
        }
    }

    void setTimer(int value) {
        timer = value;
    }

    int readTimer() {
        return timer;
    }
};

class SoundTimer : public DelayTimer {
public:
    SoundTimer() : DelayTimer() {}

    void beep() {
        if (timer > 1) {
            // Implement sound playing here
            // For now, we can just print "Beep"
            std::cout << "Beep" << std::endl;
            // Reset timer
            timer = 0;
        }
    }
};

class Emulator {
public:
    std::vector<uint8_t> Memory;
    std::vector<Register> Registers;
    Register IRegister;
    uint16_t ProgramCounter;
    std::stack<uint16_t> stack;
    DelayTimer delayTimer;
    SoundTimer soundTimer;

    // For graphics
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t pixels[64 * 32];

    // For input
    std::vector<bool> keys;
    std::map<int, uint8_t> keyDict;

    // For display
    uint8_t grid[32][64];
    uint32_t zeroColor;
    uint32_t oneColor;

    int size;

    Emulator() {
        // Initialize Memory
        Memory.resize(4096, 0);

        // Load fonts
        uint8_t fonts[80] = {
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
        for (int i = 0; i < 80; i++) {
            Memory[i] = fonts[i];
        }

        // Initialize Registers
        for (int i = 0; i < 16; i++) {
            Registers.push_back(Register(8));
        }

        // Initialize IRegister
        IRegister = Register(16);

        // Initialize ProgramCounter
        ProgramCounter = 0x200;

        // Initialize timers
        // delayTimer and soundTimer are already initialized

        // Initialize SDL
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
        size = 10;
        int width = 64;
        int height = 32;
        window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width * size, height * size, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        // Create texture
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        // Initialize keys
        keys.resize(16, false);
        keyDict = {
            {SDLK_1, 0x1},
            {SDLK_2, 0x2},
            {SDLK_3, 0x3},
            {SDLK_4, 0xC},
            {SDLK_q, 0x4},
            {SDLK_w, 0x5},
            {SDLK_e, 0x6},
            {SDLK_r, 0xD},
            {SDLK_a, 0x7},
            {SDLK_s, 0x8},
            {SDLK_d, 0x9},
            {SDLK_f, 0xE},
            {SDLK_z, 0xA},
            {SDLK_x, 0x0},
            {SDLK_c, 0xB},
            {SDLK_v, 0xF}
        };

        // Initialize grid
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 64; j++) {
                grid[i][j] = 0;
            }
        }

        zeroColor = SDL_MapRGB(SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888), 0, 0, 50);
        oneColor = SDL_MapRGB(SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888), 255, 255, 255);
    }

    ~Emulator() {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void execOpcode(uint16_t opcode) {
        // Extract nibbles
        uint8_t firstNibble = (opcode & 0xF000) >> 12;
        uint8_t secondNibble = (opcode & 0x0F00) >> 8;
        uint8_t thirdNibble = (opcode & 0x00F0) >> 4;
        uint8_t fourthNibble = (opcode & 0x000F);

        uint16_t NNN = opcode & 0x0FFF;
        uint8_t NN = opcode & 0x00FF;
        uint8_t N = opcode & 0x000F;
        uint8_t X = secondNibble;
        uint8_t Y = thirdNibble;

        switch (firstNibble) {
        case 0x0:
            if (opcode == 0x00E0) {
                // CLS
                clear();
            }
            else if (opcode == 0x00EE) {
                // RET
                ProgramCounter = stack.top();
                stack.pop();
            }
            else {
                // 0NNN - SYS addr
                std::cout << "ROM attempts to run RCA 1802 program at <0x" << std::hex << NNN << ">" << std::endl;
            }
            break;
        case 0x1:
            // JP addr
            ProgramCounter = NNN - 2;
            break;
        case 0x2:
            // CALL addr
            stack.push(ProgramCounter);
            ProgramCounter = NNN - 2;
            break;
        case 0x3:
            // SE Vx, byte
            if (Registers[X].value == NN) {
                ProgramCounter += 2;
            }
            break;
        case 0x4:
            // SNE Vx, byte
            if (Registers[X].value != NN) {
                ProgramCounter += 2;
            }
            break;
        case 0x5:
            // SE Vx, Vy
            if (Registers[X].value == Registers[Y].value) {
                ProgramCounter += 2;
            }
            break;
        case 0x6:
            // LD Vx, byte
            Registers[X].value = NN;
            break;
        case 0x7:
            // ADD Vx, byte
            Registers[X].value += NN;
            Registers[X].checkCarry();
            break;
        case 0x8:
            switch (fourthNibble) {
            case 0x0:
                // LD Vx, Vy
                Registers[X].value = Registers[Y].value;
                break;
            case 0x1:
                // OR Vx, Vy
                Registers[X].value |= Registers[Y].value;
                break;
            case 0x2:
                // AND Vx, Vy
                Registers[X].value &= Registers[Y].value;
                break;
            case 0x3:
                // XOR Vx, Vy
                Registers[X].value ^= Registers[Y].value;
                break;
            case 0x4:
                // ADD Vx, Vy
            {
                uint16_t sum = Registers[X].value + Registers[Y].value;
                Registers[0xF].value = (sum > 255) ? 1 : 0;
                Registers[X].value = sum & 0xFF;
            }
            break;
            case 0x5:
                // SUB Vx, Vy
                Registers[0xF].value = (Registers[X].value > Registers[Y].value) ? 1 : 0;
                Registers[X].value -= Registers[Y].value;
                break;
            case 0x6:
                // SHR Vx {, Vy}
                Registers[0xF].value = Registers[X].value & 0x1;
                Registers[X].value >>= 1;
                break;
            case 0x7:
                // SUBN Vx, Vy
                Registers[0xF].value = (Registers[Y].value > Registers[X].value) ? 1 : 0;
                Registers[X].value = Registers[Y].value - Registers[X].value;
                break;
            case 0xE:
                // SHL Vx {, Vy}
                Registers[0xF].value = (Registers[X].value & 0x80) >> 7;
                Registers[X].value <<= 1;
                break;
            default:
                std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
                break;
            }
            break;
        case 0x9:
            // SNE Vx, Vy
            if (Registers[X].value != Registers[Y].value) {
                ProgramCounter += 2;
            }
            break;
        case 0xA:
            // LD I, addr
            IRegister.value = NNN;
            break;
        case 0xB:
            // JP V0, addr
            ProgramCounter = Registers[0].value + NNN - 2;
            break;
        case 0xC:
            // RND Vx, byte
            Registers[X].value = (rand() % 256) & NN;
            break;
        case 0xD:
            // DRW Vx, Vy, nibble
        {
            std::vector<uint8_t> sprite(N);
            for (int i = 0; i < N; i++) {
                sprite[i] = Memory[IRegister.value + i];
            }
            if (draw(Registers[X].value, Registers[Y].value, sprite)) {
                Registers[0xF].value = 1;
            }
            else {
                Registers[0xF].value = 0;
            }
        }
        break;
        case 0xE:
            if (NN == 0x9E) {
                // SKP Vx
                if (keys[Registers[X].value]) {
                    ProgramCounter += 2;
                }
            }
            else if (NN == 0xA1) {
                // SKNP Vx
                if (!keys[Registers[X].value]) {
                    ProgramCounter += 2;
                }
            }
            break;
        case 0xF:
            switch (NN) {
            case 0x07:
                // LD Vx, DT
                Registers[X].value = delayTimer.readTimer();
                break;
            case 0x0A:
                // LD Vx, K
            {
                bool keyPressed = false;
                while (!keyPressed) {
                    keyHandler();
                    for (size_t i = 0; i < keys.size(); i++) {
                        if (keys[i]) {
                            Registers[X].value = i;
                            keyPressed = true;
                            break;
                        }
                    }
                }
            }
            break;
            case 0x15:
                // LD DT, Vx
                delayTimer.setTimer(Registers[X].value);
                break;
            case 0x18:
                // LD ST, Vx
                soundTimer.setTimer(Registers[X].value);
                break;
            case 0x1E:
                // ADD I, Vx
                IRegister.value += Registers[X].value;
                break;
            case 0x29:
                // LD F, Vx
                IRegister.value = Registers[X].value * 5;
                break;
            case 0x33:
                // LD B, Vx
            {
                uint8_t value = Registers[X].value;
                Memory[IRegister.value + 2] = value % 10;
                value /= 10;
                Memory[IRegister.value + 1] = value % 10;
                value /= 10;
                Memory[IRegister.value] = value % 10;
            }
            break;
            case 0x55:
                // LD [I], Vx
                for (int i = 0; i <= X; i++) {
                    Memory[IRegister.value + i] = Registers[i].value;
                }
                break;
            case 0x65:
                // LD Vx, [I]
                for (int i = 0; i <= X; i++) {
                    Registers[i].value = Memory[IRegister.value + i];
                }
                break;
            default:
                std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
                break;
            }
            break;
        default:
            std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
            break;
        }

        ProgramCounter += 2;
    }

    void execution() {
        // Fetch opcode
        uint16_t opcode = Memory[ProgramCounter] << 8 | Memory[ProgramCounter + 1];
        execOpcode(opcode);
    }

    bool draw(uint8_t Vx, uint8_t Vy, std::vector<uint8_t>& sprite) {
        bool collision = false;

        for (size_t i = 0; i < sprite.size(); i++) {
            uint8_t spriteByte = sprite[i];
            for (int j = 0; j < 8; j++) {
                uint8_t spritePixel = (spriteByte >> (7 - j)) & 0x1;
                uint8_t& screenPixel = grid[(Vy + i) % 32][(Vx + j) % 64];
                if (spritePixel == 1) {
                    if (screenPixel == 1) {
                        collision = true;
                    }
                    screenPixel ^= 1;
                }
            }
        }

        return collision;
    }

    void clear() {
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 64; j++) {
                grid[i][j] = 0;
            }
        }
    }

    void readProg(const char* filename) {
        std::vector<uint8_t> rom = convertProg(filename);
        int offset = 0x200;
        for (size_t i = 0; i < rom.size(); i++) {
            Memory[offset + i] = rom[i];
        }
    }

    std::vector<uint8_t> convertProg(const char* filename) {
        std::vector<uint8_t> rom;
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open ROM file: " << filename << std::endl;
            exit(1);
        }
        char byte;
        while (file.get(byte)) {
            rom.push_back(static_cast<uint8_t>(byte));
        }
        return rom;
    }

    void keyHandler() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            else if (event.type == SDL_KEYDOWN) {
                auto it = keyDict.find(event.key.keysym.sym);
                if (it != keyDict.end()) {
                    keys[it->second] = true;
                }
            }
            else if (event.type == SDL_KEYUP) {
                auto it = keyDict.find(event.key.keysym.sym);
                if (it != keyDict.end()) {
                    keys[it->second] = false;
                }
            }
        }
    }

    void mainLoop() {
        SDL_Event event;
        uint32_t startTick;
        while (true) {
            startTick = SDL_GetTicks();

            keyHandler();
            soundTimer.beep();
            execution();
            display();

            delayTimer.countDown();

            // 60Hz timing
            uint32_t frameTime = SDL_GetTicks() - startTick;
            if (frameTime < (1000 / 60)) {
                SDL_Delay((1000 / 60) - frameTime);
            }
        }
    }

    void display() {
        // Update pixels array
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 64; j++) {
                uint32_t color = (grid[i][j] == 1) ? oneColor : zeroColor;
                pixels[i * 64 + j] = color;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: chip8 [ROM file]" << std::endl;
        return 1;
    }

    // Seed the random number generator
    srand(static_cast<unsigned int>(time(NULL)));

    Emulator chip8;
    chip8.readProg(argv[1]);
    chip8.mainLoop();

    return 0;
}
