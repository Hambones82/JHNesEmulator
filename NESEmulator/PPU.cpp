#include <stdint.h>

import NES_Memory_System;
import PPU;

uint8_t PPU::ReadAddr(uint16_t address) {
	return memory_system->FetchByte(address);
}