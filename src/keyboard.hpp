#ifndef CHIP8_KEYBOARD_H
#define CHIP8_KEYBOARD_H

#include <SDL2/SDL.h>


// It's better to use scancode than keycode 
// because it's not sensible to the keyboard region (e.g. QWERTY or AZERTY)
int keyboardMapping(SDL_Scancode scancode);

#endif