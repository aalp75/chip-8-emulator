build:
	g++ -std=c++23 src/run.cpp src/chip8.cpp src/keyboard.cpp src/audio.cpp -o run.out -I/usr/include/SDL2 -D_REENTRANT -lSDL2