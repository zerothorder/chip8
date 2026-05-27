#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#define SDL_MAIN_HANDLED
#define SCALE 2

typedef struct Chip8 {
    uint8_t memory[4096];
    uint8_t registers[16];
    uint16_t indexRegister;
    uint16_t programCounter;
    uint16_t stack[16];
	uint8_t stackPointer;
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t screen[64 * 32];
    uint8_t keys[16];
} Chip8;

uint8_t fontset[80] =
{
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

const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

void init(Chip8 *emu) {
	memset(emu, 0, sizeof(Chip8));
    emu->programCounter = START_ADDRESS;

	for (unsigned int i = 0; i < 80; ++i) {
            emu->memory[FONTSET_START_ADDRESS + i] = fontset[i];
        }

}

void loadROM(Chip8 *emu, const char *filename) {
	FILE *f = fopen(filename, "rb");
	if(f == NULL) {
		printf("Error: Could not open file %s\n", filename);
		return;
	}
	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	//reset file pointer back to the beginning
	rewind(f);
	fread(emu->memory + 0x200, 1, fileSize, f);
	fclose(f);
}

void cycle(Chip8 *emu) {
	//fetch
	uint16_t opcode = (uint16_t)(emu->memory[emu->programCounter] << 8 | emu->memory[emu->programCounter+1]);
	emu->programCounter += 2;

	//decode(?)
	uint16_t firstNibble = opcode & 0xF000;
	uint16_t X = (opcode & 0x0F00) >> 8;
	uint16_t Y = (opcode & 0x00F0) >> 4;
	uint16_t N = opcode & 0x000F;
	uint16_t NN = opcode & 0x00FF;
	uint16_t NNN = opcode & 0x0FFF;
	switch(firstNibble) {
		case 0x0000:
			switch(NN) {
				case 0xE0: //cls - clear display
					memset(emu->screen, 0, sizeof(emu->screen));
					break;
			}
			break;
		case 0x1000:
			emu->programCounter = NNN;
			break;
		case 0x2000:
			break;
		case 0x3000:
			break;
		case 0x4000:
			break;
		case 0x5000:
			break;
		case 0x6000:
			emu->registers[X] = NN;
			break;
		case 0x7000:
			emu->registers[X] += NN;
			break;
		case 0x8000:
			break;
		case 0x9000:
			break;
		case 0xA000:
			emu->indexRegister = NNN;
			break;
		case 0xB000:
			break;
		case 0xC000:
			break;
		case 0xD000: {
			emu->registers[0xF] = 0;
			for (int row = 0; row < N; row++) {
				uint8_t spriteByte = emu->memory[emu->indexRegister + row];

				for (int col = 0; col < 8; col++) {
					// check if this bit is set in spriteByte
					uint8_t spritePixel = spriteByte & (0x80 >> col);
					// find the screen pixel position
					uint8_t xPOS = emu->registers[X] + col;
					uint8_t yPOS = emu->registers[Y] + row;
					const unsigned int index = yPOS * 64 + xPOS; 

					if(spritePixel) {
						// check for collision -> set VF
						if (emu->screen[index]) {
							emu->registers[0xF] = 1;
						}
						// XOR it
						emu->screen[index] ^= 1;
					}
				}
			}
			break;
		}
		case 0xE000:
			break;
		case 0xF000:
			break;

	}

}

int main(int argc, char* argv[]) {
	Chip8 emulator;
	init(&emulator);
	// SDL_Init(SDL_INIT_VIDEO);
    // SDL_Window* window = SDL_CreateWindow("Chip8",
    //     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    //     64*SCALE, 32*SCALE, SDL_WINDOW_SHOWN);

    // SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // SDL_Texture* Texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, N, N);

	
	// int simulation_running = 1;
    // SDL_Event event;
    // while(simulation_running) {
	// 	//fetch-decode cycle
	// 	;
	// }
}