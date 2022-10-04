#include <iostream>
#include <stdint.h>
#include <array>

import NES_Memory_System;
import PPU;

uint8_t PPU::ReadAddr(uint16_t address) {
	return memory_system->FetchPPUByte(address);
}

void PPU::WriteAddr(uint16_t address, uint8_t value) {
	memory_system->WritePPUByte(address, value);
}

const std::array<PPU::Color, 64> PPU::MasterPallette = {
	//https://www.nesdev.org/wiki/PPU_palettes
	//row1
	Color{84,84,84}, Color{0,30,116},Color{8,16,144},Color{48,0,136},
	Color{68,0,100},Color{92,0,49},Color{84,4,0},Color{60,24,0},
	Color{32,42,0},Color{8,58,0},Color{0,64,0},Color{0,60,0},
	Color{0,50,60},Color{0,0,0},Color{0,0,0},Color{0,0,0},
	//row2
	Color{152,150,152}, Color{8,76,196},Color{48,50,236},Color{92,30,228},
	Color{136,20,176},Color{160,20,100},Color{152,34,32},Color{120,60,0},
	Color{84,90,0},Color{40,114,0},Color{8,124,0},Color{0,118,40},
	Color{0,102,120},Color{0,0,0},Color{0,0,0},Color{0,0,0},
	//row3
	Color{236,238,236},Color{76,154,236},Color{120,124,236},Color{176,98,236},
	Color{228,84,236},Color{236,88,180},Color{236,106,100},Color{212,136,32},
	Color{160,170,0},Color{116,196,0},Color{76,208,32},Color{56,204,108},
	Color{56,180,204},Color{60,60,60},Color{0,0,0},Color{0,0,0},
	//row4
	Color{236,238,236},Color{168,204,236},Color{188,188,236},Color{212,178,236},
	Color{236,174,236},Color{236,174,212},Color{236,180,176},Color{228,196,144},
	Color{204,210,120},Color{180,222,120},Color{168,226,144},Color{152,226,180},
	Color{160,214,228},Color{160,162,160},Color{0,0,0},Color{0,0,0}
};

void PPU::DisplayNT() {
	std::cout << memory_system->PPURange(0x2000, 0x800, 32);
}