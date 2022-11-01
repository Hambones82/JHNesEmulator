module;
#include <stdint.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include<array>
export module NESROMLoader;
import NES_Memory_System;

export union {
	uint8_t bytes[15];
	struct {
		uint8_t signature[4];
		uint8_t PRG_ROM_size;
		uint8_t CHR_ROM_size;
		struct {
			uint8_t mirroring : 1;
			uint8_t persisten_mem : 1;
			uint8_t trainer : 1;
			uint8_t ignore_mirroring : 1;
			uint8_t mapper_num_lower_nibble : 4;
		}info;
		uint8_t unused[9];
	}flags;
}NES_ROM_header;

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
			std::vector<uint8_t> header_in(first, last);
			
			memcpy(NES_ROM_header.bytes, header_in.data(), 16);

			NT_mirroring_mode m_mode = NES_ROM_header.flags.info.mirroring ? NT_mirroring_mode::vertical : NT_mirroring_mode::horizontal;
			memory_system->SetMirroringMode(m_mode);

			uint8_t PRG_size = NES_ROM_header.flags.PRG_ROM_size; // size in 16k chunks, total of 2 banks.
			uint8_t CHR_size = NES_ROM_header.flags.CHR_ROM_size; // size in 8k chunks

			if (PRG_size == 1)
			{
				first = file_buf.begin() + 0x10;
				last = file_buf.begin() + 0x2010;
				std::vector<uint8_t> bank0(first, last);
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
			}
			else if (PRG_size == 2) {
				first = file_buf.begin() + 0x10;
				last = file_buf.begin() + 0x2010;
				std::vector<uint8_t> bank0(first, last);
				first = file_buf.begin() + 0x2010;
				last = file_buf.begin() + 0x4010;
				std::vector<uint8_t> bank1(first, last);
				first = file_buf.begin() + 0x4010;
				last = file_buf.begin() + 0x6010;
				std::vector<uint8_t> bank2(first, last);
				first = file_buf.begin() + 0x6010;
				last = file_buf.begin() + 0x8010;
				std::vector<uint8_t> bank3(first, last);
				first = file_buf.begin() + 0x8010;
				last = file_buf.begin() + 0xA010;
				std::vector<uint8_t> chrbank0(first, last);

				memory_system->LoadPRGBank(bank0, 1);
				memory_system->MapPRGBank(0x8000, 1);
				memory_system->LoadPRGBank(bank1, 2);
				memory_system->MapPRGBank(0xA000, 2);
				memory_system->LoadPRGBank(bank2, 3);
				memory_system->MapPRGBank(0xC000, 3);
				memory_system->LoadPRGBank(bank3, 4);
				memory_system->MapPRGBank(0xE000, 4);
				memory_system->LoadCHRBank(chrbank0, 1);
				memory_system->MapCHRBank(0, 1);
			}

			
			
		}
		else {
			std::cout << "Could not open file: " << file_name << std::endl;
			char cont = 0;
			std::cin >> cont;
		}
	}
};