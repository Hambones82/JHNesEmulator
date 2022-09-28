import RenderingWindow;
import NES_Memory_System;
import NESCPU;
import NESROMLoader;
import nestestloader;
import PPU;
import MasterClock;
import PlatformIO;
import NESIO;

#include <memory>
#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

int main(int argc, char* args[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	RenderingWindow nesRenderer{};
	//RenderingWindow patternTablewindow{};
	PPU ppu(&nesRenderer);
	PlatformIO platformIO;
	NESIO nesIO{ &platformIO };
	NES_Memory_System memory{&ppu, &nesIO};
	ppu.SetMemorySystem(&memory);
	MasterClock masterClock(&ppu);
	
	NESCPU cpu{ &memory , &masterClock};
	NESROMLoader::LoadRomFile(R"(C:\Users\Josh\source\NESEmulator\Roms\nestest.nes)", &memory);

	NEStestloader nestestloader;
	nestestloader.Init();

	int y_offset = 1;
	/*
	nesRenderer.StartFrame();
	uint8_t tile_row;
	uint8_t tile_col;
	for (int side = 0; side < 2; side++) {
		for (tile_row = 0; tile_row < 16; tile_row++) {
			for (tile_col = 0; tile_col < 16; tile_col++) {
				uint16_t tile_mask = (tile_row << 8) | (tile_col << 4);
				for (int row = 0; row < 8; row++) {
					uint8_t ored_byte = memory.FetchPPUByte(tile_mask + row + side * 0x1000) | memory.FetchPPUByte(tile_mask + row + 8 + side * 0x1000);
					for (int col_offset = 0; col_offset < 8; col_offset++) {
						int y = (tile_row << 3) + row + y_offset;
						int x = (tile_col << 3) + (8 - col_offset);
						int current_color = (ored_byte & (1 << col_offset)) >> col_offset;
						nesRenderer.SetColor(current_color * 255, current_color * 255, current_color * 255, 0);
						nesRenderer.DrawPixel(x + side * 128, y);
					}
				}
			}
		}
	}
	nesRenderer.EndFrame();
	//do addresses 0 to FFF first
	*/

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {  // poll until all events are handled!
			// decide what to do with this event.
			
		}
		cpu.AdvanceExecution();
		// update game state, draw the current frame
		
	}

	const int startProg = 0x0;
	const int startData = 0x100;
	
	std::string input = "";
	std::string quit = "q";
	int num_instructions = 0;
	int instruction_num_target = -1;
	bool fast_forward = false;
	bool pause = false;
	int mem_page = 0;
	while (input != quit)
	{
		if (!fast_forward) {
			std::cout << "Instructions processed: " << num_instructions << std::endl;
			std::cout << "Instruction number: " << std::dec << num_instructions << std::endl;
			std::cout << "Registers: \n===================\n";
			std::cout << cpu.GetState() << "\n\n";
			std::cout << "Stack: \n====================\n";
			std::cout << memory.GetRange(0x0100, 0x0100, 32) << "\n\n";
			std::cout << "Memory: \n====================\n";
			std::cout << memory.GetRange(mem_page << 8, 0x0100, 32) << "\n\n";
			
			std::cin >> input;
		}
		
		if (std::all_of(input.begin(), input.end(), std::isdigit)) {
			instruction_num_target = std::stoi(input);
			fast_forward = true;
			pause = false;
		}
		else if (input[0] == 'm') {
			if (input.size() >= 2) {
				if (std::all_of(input.begin() + 1, input.end(), std::isxdigit)) {
					mem_page = hexStrtoInt(input.substr(1));
					pause = true;
				}
			}
		}
		
		else {
			fast_forward = false;
			pause = false;
		}
			
		NESCPU_state cpu_state = cpu.GetStateObj();
		if (!(cpu_state == nestestloader.GetStateSequence(num_instructions))) {
			std::cout << "line mismatch\n";
			fast_forward = false;
			instruction_num_target = 0;
			pause = true;
		}

		//advance execution and counter
		if (!pause) {
			cpu.AdvanceExecution();
			num_instructions++;
		}
		

		//stop fast forward if reach limit
		if (instruction_num_target <= num_instructions)
		{
			fast_forward = false;
		}
		
		//print out display if not fast forward
		
		if (!fast_forward && !pause) {
			system("cls");
		}
	}
	
	return 0;
}