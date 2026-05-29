#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#define SDL_MAIN_HANDLED
#define SCALE 20

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
				case 0xEE: //ret - return from subroutine
					--emu->stackPointer;
					emu->programCounter = emu->stack[emu->stackPointer];
			}
			break;
		case 0x1000:
			emu->programCounter = NNN;
			break;
		case 0x2000: //call sub routine at NNN
			emu->stack[emu->stackPointer] = emu->programCounter; 
			emu->stackPointer++;
			emu->programCounter = NNN;
			break;
		case 0x3000: //Skip next instruction if Vx = NN.
			if(emu->registers[X] == NN) {
				emu->programCounter += 2;
			}
			break;
		case 0x4000: //Skip next instruction if Vx != NN.
			if(emu->registers[X] != NN) {
				emu->programCounter += 2;
			}
			break;
		case 0x5000: //skip if Vx = Vy
			if(emu->registers[X] == emu->registers[Y]) {
				emu->programCounter += 2;
			}
			break;
		case 0x6000:
			emu->registers[X] = NN;
			break;
		case 0x7000:
			emu->registers[X] += NN;
			break;
		case 0x8000:
			switch(N) {
				case 0x0: //set
					emu->registers[X] = emu->registers[Y];
					break; 
				case 0x1: //binary OR
					emu->registers[X] |= emu->registers[Y];
					break; 
				case 0x2: //binary AND
					emu->registers[X] &= emu->registers[Y];
					break;
				case 0x3: //logical XOR
					emu->registers[X] ^= emu->registers[Y];
					break;
				case 0x4: //add
				{
					uint16_t sum = emu->registers[X] + emu->registers[Y];
					emu->registers[X] = sum;
					emu->registers[0xF] = (sum > 0xFF) ? 1 : 0;
					break;
				}
				case 0x5: //subtract
				{
					uint8_t vx = emu->registers[X];
					uint8_t vy = emu->registers[Y];
					emu->registers[X] = vx - vy;
					emu->registers[0xF] = (vx >= vy) ? 1 : 0;
					break;
				}
				case 0x6: 
				{
					uint8_t lsb = emu->registers[Y] & 0x1;
					emu->registers[X] = emu->registers[Y] >> 1;
					emu->registers[0xF] = lsb;
					break;
				}
				case 0x7: //reverse difference of 0x5
				{
					uint8_t vx = emu->registers[X];
					uint8_t vy = emu->registers[Y];
					emu->registers[X] = vy - vx;
					emu->registers[0xF] = (vy >= vx) ? 1 : 0;
					break;
				}	
				case 0xE:
				{
					uint8_t msb = (emu->registers[Y] >> 7) & 0x01;
					emu->registers[X] = emu->registers[Y] << 1;
					emu->registers[0xF] = msb;
					break;
				}
			}
			break;
		case 0x9000: //skip if Vx != Vy
			if(emu->registers[X] != emu->registers[Y]) {
				emu->programCounter += 2;
			}
			break;
		case 0xA000:
			emu->indexRegister = NNN;
			break;
		case 0xB000: //Jump with offset
			emu->programCounter = NNN + emu->registers[0];
			break;
		case 0xC000: //random
		{	
			emu->registers[X] = (rand() % 256) & NN;
			break;
		}
		case 0xD000: {
			emu->registers[0xF] = 0;
			for (int row = 0; row < N; row++) {
				uint8_t spriteByte = emu->memory[emu->indexRegister + row];

				for (int col = 0; col < 8; col++) {
					//check if this bit is set in spriteByte
					uint8_t spritePixel = spriteByte & (0x80 >> col);
					//find the screen pixel position
					uint8_t xPOS = emu->registers[X] + col;
					uint8_t yPOS = emu->registers[Y] + row;
					const unsigned int index = yPOS * 64 + xPOS; 
					
					//DXYN clipping
					if(xPOS>=64 || yPOS >=32) {
						continue;
					}

					if(spritePixel) {
						//check for collision - set VF
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
			switch(NN) {
				case 0x9E:
					break;
				case 0xA1:
					break;
			}
			break;
		case 0xF000:
			switch(NN) {
				case 0x07:
					emu->registers[X] = emu->delayTimer;
					break;
				case 0x0A:
				{
					int keyFound = -1;
					for (int i = 0; i < 16; i++) {
						if (emu->keys[i]) {
							keyFound = i;
							break;
						}
					}
					if (keyFound == -1) {
						emu->programCounter -= 2; 
					} else {
						emu->registers[X] = keyFound;
					}
					break;
				}
					break;
				case 0x15:
					emu->delayTimer = emu->registers[X];
					break;
				case 0x18:
					emu->soundTimer = emu->registers[X];
					break;
				case 0x1E:
					emu->indexRegister += emu->registers[X];
					break;
				case 0x29:
					emu->indexRegister = FONTSET_START_ADDRESS + (emu->registers[X] & 0xF) * 5;
					break;
				case 0x33:
				{
					uint8_t value = emu->registers[X];
					//Ones-place
					emu->memory[emu->indexRegister + 2] = value % 10;
					value /= 10;

					//Tens-place
					emu->memory[emu->indexRegister + 1] = value % 10;
					value /= 10;

					//Hundreds-place
					emu->memory[emu->indexRegister] = value % 10;
					break;
				}
				case 0x55:
				{
					for(int i = 0; i <= X; i++) {
						emu->memory[emu->indexRegister + i] = emu->registers[i];
					}
					break;
				}
				case 0x65:
					for(int i = 0; i <= X; i++) {
						emu->registers[i] = emu->memory[emu->indexRegister + i];
					}
					break;
			}
			break;

	}

}

int main(int argc, char* argv[]) {
	Chip8 emulator;
	srand(time(NULL));
	init(&emulator);
	loadROM(&emulator, argv[1]);

	SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Chip8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        64*SCALE, 32*SCALE, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


	int simulation_running = 1;
    SDL_Event event;
    while (simulation_running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) simulation_running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) simulation_running = 0;
        }
		cycle(&emulator);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for (int i = 0; i < 64 * 32; i++) {
			if (emulator.screen[i]) {
				SDL_Rect rect = {
					(i % 64) * SCALE,
					(i / 64) * SCALE,
					SCALE, SCALE
				};
				SDL_RenderFillRect(renderer, &rect);
			}
		}

		SDL_RenderPresent(renderer);

		}

	return 0;
}