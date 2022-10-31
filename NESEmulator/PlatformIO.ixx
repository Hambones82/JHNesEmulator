module;
#include <SDL.h>
#include <stdint.h>
export module PlatformIO;

export class PlatformIO {
private:
	const Uint8* KeyboardState; //pointer to SDL's keyboard state

public:
	bool GetA() { return KeyboardState[SDL_SCANCODE_X]; }
	bool GetB() { return KeyboardState[SDL_SCANCODE_Z]; }
	bool GetSelect() { return KeyboardState[SDL_SCANCODE_RSHIFT]; }
	bool GetStart() { return KeyboardState[SDL_SCANCODE_RETURN]; }
	bool GetUp() { return KeyboardState[SDL_SCANCODE_UP]; }
	bool GetDown() { return KeyboardState[SDL_SCANCODE_DOWN]; }
	bool GetLeft() { return KeyboardState[SDL_SCANCODE_LEFT]; }
	bool GetRight() { return KeyboardState[SDL_SCANCODE_RIGHT]; }

	PlatformIO() {
		KeyboardState = SDL_GetKeyboardState(NULL);
	}
};