#include <thread>
#include <memory>
#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

import NESMain;

int main(int argc, char* args[])
{
	std::ios_base::sync_with_stdio(false);
	SDL_Init(SDL_INIT_EVERYTHING);

	NESData nesData{};
	

	//audioDriver.Reset();
	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {  // poll until all events are handled!
		}
		nesData.AdvanceCPU();
	}
	
	return 0;
}