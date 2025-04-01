#include <iostream>
#include <fstream>
#include <cstdint>
#include <SDL2/SDL.h>

#include "ops.h"

#define SCREEN_HEIGHT 500
#define SCREEN_WIDTH 500

#define RAM_ADDRESS 128
#define ROM_ADDRESS 4096
#define MEMORY_SIZE 8192

void read_program(char* memory, char* program_file) {
    std::ifstream file(program_file, std::ifstream::in);
    if (file.is_open()) {
        file.seekg(0, file.end);
        int file_length = file.tellg();
        if (file_length > MEMORY_SIZE - ROM_ADDRESS) {
            // error handling here
        }
        file.seekg(0, file.beg);
        file.read(memory + ROM_ADDRESS, file_length);
        file.close();
    } else {
        // error handling here
    }
}

int main(int argc, char** argv) {
    SDL_Window* window;
    SDL_Surface* surface;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "sdl error: " << SDL_GetError() << "\n";
    }

    window = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cout << "sdl error: " << SDL_GetError() << "\n";
    }

    surface = SDL_GetWindowSurface(window);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);

    char memory[8191];
    char flags = 0xff;
    short pc = 0;
    char sp = 127;
    read_program(memory, argv[1]);

    // main loop
    char opcode, arg1, arg2;
    while (1) {
        opcode = memory[pc + ROM_ADDRESS];
        if (ops_noargs.count(opcode)) {
            ops_noargs.at(opcode)();
            pc++;
        } else if (ops_onearg.count(opcode)) {
            arg1 = memory[pc + ROM_ADDRESS + 1];
            ops_onearg.at(opcode)(arg1);
            pc += 2;
        } else if (ops_twoargs.count(opcode)) {
            arg1 = memory[pc + ROM_ADDRESS + 1];
            arg2 = memory[pc + ROM_ADDRESS + 2];
            ops_twoargs.at(opcode)(arg1, arg2);
            pc += 3;
        } else {
            // error handling
        }
    }

    return 0;
}