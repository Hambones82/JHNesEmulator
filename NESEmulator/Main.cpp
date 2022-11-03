#include <thread>
#include <memory>
#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include "NESData.h"


int main(int argc, char* args[])
{
	std::ios_base::sync_with_stdio(false);
	SDL_Init(SDL_INIT_EVERYTHING);

	NESData nesData;
	
	while (true) {
		
		nesData.AdvanceCPU();
	}
	
	return 0;
}