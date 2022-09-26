import NES_Memory_System;

#include <stdint.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include<array>
export module NESROMLoader;

export class NESROMLoader {
	//fetch header...
public:
	static void LoadRomFile(std::string file_name, NES_Memory_System* memory_system) {
		std::ifstream ROMtoLoad;
		
		ROMtoLoad.open(file_name, std::ios_base::binary);
		if (ROMtoLoad.good()) {
			std::vector<uint8_t> file_buf((std::istreambuf_iterator<char>(ROMtoLoad)), (std::istreambuf_iterator<char>()));
			std::vector<uint8_t>::const_iterator first = file_buf.begin();
			std::vector<uint8_t>::const_iterator last = file_buf.begin() + 15;
			std::vector<uint8_t> header(first, last);
			
			first = file_buf.begin() + 16;
			last = file_buf.begin() + 0x2010;
			std::vector<uint8_t> bank0 (first, last);
			first = file_buf.begin() + 0x2010;
			last = file_buf.begin() + 0x4010;
			std::vector<uint8_t> bank1(first, last);
			first = file_buf.begin() + 0x4010;
			last = file_buf.begin() + 0x6010;
			std::vector<uint8_t> chrbank0(first, last);
			
			memory_system->LoadPRGBank(bank0, 1);
			memory_system->MapPRGBank(0x8000, 1);
			memory_system->MapPRGBank(0xC000, 1);
			memory_system->LoadPRGBank(bank1, 2);
			memory_system->MapPRGBank(0xA000, 2);
			memory_system->MapPRGBank(0xE000, 2);
			memory_system->LoadCHRBank(chrbank0, 1);
			memory_system->MapCHRBank(0, 1);
			
			//need to load chr rom but don't worry about that at just the cpu stage
		}
		else {
			std::cout << "\Could not open file: " << file_name << std::endl;
			char cont = 0;
			std::cin >> cont;
		}
	}
};