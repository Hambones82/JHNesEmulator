#include <cstdint>
#include <array>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include<iomanip>

import PPU;
import NESIO;
import CartridgeMem;
export module NES_Memory_System;

const uint16_t banks_in_addr_space = UINT16_MAX +1 / bank_size;
const uint16_t max_banks = 32;

const uint16_t base_addr_mask = 0xE000; //top 3 bits...  address space size (64KB) / bank size (8KB) = 8
const uint16_t offset_mask = 0x1FFF;
const uint8_t address_shift_amount = 13;//conversion between base addr and base num

export typedef std::array<uint8_t, bank_size> bank; //bank loaded from a file.  prg_rom are split across 2 of these.
typedef std::array<uint8_t, 1024> name_table;

//module scope -- actually, i have no idea what the scope of these declarations is.
CartridgeMem PRGROM;
CartridgeMem CHRROM;

export enum class NT_mirroring_mode {vertical, horizontal, dual};

export class NES_Memory_System
{
private:
	PPU* ppu;
	NESIO* nesIO;
	std::array<uint8_t, 65536> RAM = { 0 };
	//std::array<uint8_t, 1024> NameTable1 = { 0 };
	//std::array<uint8_t, 1024> NameTable2 = { 0 };
	std::array<name_table, 2> NameTables;
	NT_mirroring_mode mirroring_mode;
	std::array<uint8_t, 4> mirroring_page_table = { 0 };
	std::array<uint8_t, 0x20> PPU_Pallette_RAM = { 0 };
	std::array<uint16_t, banks_in_addr_space> page_table; //provides base addresses
														  //usage: page_table[page_num] --> base addr.

	void WriteRawAddr(uint16_t address, uint8_t data) {
		if (address < 0x2000) {
			RAM[address] = data;
		}
		else if (address >= 0x2000 && address < 0x4000) {
			ppu->WriteReg(address & 0x0007, data);
		}
		else if (address == 0x4014) {
			//OAM DMA
		}
		else if (address == 0x4016) {
			nesIO->writeIOReg(IOReg::reg4016, data);//initialize controller input read
		}
		else {
			PRGROM.WriteROM(address, data);
		}
	}
	uint8_t ReadRawAddr(uint16_t address) {
		if (address < 0x2000)
		{
			return RAM[address];
		}
		else if (address >= 0x2000 && address < 0x4000) {
			uint8_t read_val = ppu->ReadReg(address & 0x0007);
			//std::cout << "reading: " << address << "val: " << (int)read_val << "\n";
			return read_val;
		}
		else if (address == 0x4016) {
			return nesIO->readIOReg(IOReg::reg4016);
		}
		else if (address == 0x4016) {
			//get input state
		}
		else {
			return PRGROM.ReadROM(address);
		}
	}

	uint8_t ReadRawPPUAddr(uint16_t address) {
		if ((address >= 0) && (address < 0x2000)) {
			return CHRROM.ReadROM(address);
		}
		else if ((address >= 0x2000) && (address < 0x3000)) {
			uint8_t page_num = (address & 0b0000'1100'0000'0000) >> 10;
			//std::cout << (int)page_num;
			uint16_t offset = (address & 0b0000'0011'1111'1111);
			//std::cout << (int)offset;
			uint8_t retval = NameTables[mirroring_page_table[page_num]][offset];
			//std::cout << (int)retval << "\n";
			return retval;
		}
		else if ((address >= 0x3F00) && (address < 0x4000)) {
			return PPU_Pallette_RAM[(address & 0x1F)];
		}
		else return 0;
	}

	void WriteRawPPUAddr(uint16_t address, uint8_t value) {
		if ((address >= 0x2000) && (address < 0x3000)) {

			//std::cout << "writing to vram\n";
			//write to vram
			//donkey kong has horizontal mirroring
			//2000 = 2400 -- nt1
			//2800 = 2c00 -- nt2
			//array address = address & 0x03FF
			//this is wrong...
			uint8_t page_num = (address & 0b0000'1100'0000'0000) >> 10;
			uint16_t offset = (address & 0b0000'0011'1111'1111);
			NameTables[mirroring_page_table[page_num]][offset] = value;
		}
		else if ((address >= 0x3F00) && (address < 0x4000)) {
			PPU_Pallette_RAM[(address & 0x1F)] = value;
		}
	}

public:
	//ppu map, ppu fetch functions
	//maybe we could start off with a pattern rom viewer.  that'd probably 
	
	NES_Memory_System(PPU* in_PPU, NESIO* in_nesIO) {
		ppu = in_PPU;
		nesIO = in_nesIO;
	}

	bool GetNMI() {
		bool nmi = ppu->GetNMI();
		//if (nmi) {
		//	std::cout << GetRange(0x0200, 0x0100, 32);
		//}
		return nmi;
	}

	uint8_t FetchByte(uint16_t address)
	{
		return ReadRawAddr(address);
	}
	uint16_t FetchWord(uint16_t address)
	{
		if (address == 65535)
			return ReadRawAddr(address);
		return (ReadRawAddr(address+1) << 8) |
			ReadRawAddr(address);
	}
	void StoreByte(uint16_t address, uint8_t data)
	{
		WriteRawAddr(address, data);
	}
	void StoreWord(uint16_t address, uint16_t data)
	{
		WriteRawAddr(address, data >> 8);
		WriteRawAddr(address + 1, data & 0x00FF);
	}
	
	uint8_t FetchPPUByte(uint16_t address) {
		return ReadRawPPUAddr(address);
	}

	void WritePPUByte(uint16_t address, uint8_t value) {
		WriteRawPPUAddr(address, value);
	}

	void LoadPRGBank(std::vector<uint8_t> &in_Bank, int bank_num) {
		PRGROM.LoadBank(in_Bank, bank_num);
	}
	void MapPRGBank(uint16_t new_base_addr, uint8_t bank_num) {
		PRGROM.MapBank(new_base_addr, bank_num);
	}

	void LoadCHRBank(std::vector<uint8_t>& in_Bank, int bank_num) {
		CHRROM.LoadBank(in_Bank, bank_num);
	}

	void MapCHRBank(uint16_t new_base_addr, uint8_t bank_num) {
		CHRROM.MapBank(new_base_addr, bank_num);
	}

	void SetMirroringMode(NT_mirroring_mode mmode) {
		mirroring_mode = mmode;
		if (mmode == NT_mirroring_mode::vertical) {
			mirroring_page_table[0] = 0;
			mirroring_page_table[1] = 0;
			mirroring_page_table[2] = 1;
			mirroring_page_table[3] = 1;

		}
		else if (mmode == NT_mirroring_mode::horizontal) {
			mirroring_page_table[0] = 0;
			mirroring_page_table[1] = 1;
			mirroring_page_table[2] = 0;
			mirroring_page_table[3] = 1;
		}
	}

	std::string GetRange(uint16_t begin, uint16_t length, uint8_t width)
	{
		std::string output;
		std::stringstream ss;
		int alignmentI = 0;
		for (int i = begin; i < begin + length; i++)
		{
			if (alignmentI % width == 0)
			{
				ss << std::setw(4) << std::setfill('0') << std::hex << i << ":";
			}
			ss << " " << std::setw(2) << std::setfill('0') << std::hex << (int)ReadRawAddr(i);
			if (alignmentI % width == width - 1)
			{
				ss << "\n";
			}
			alignmentI++;
		}
		output = ss.str();
		return output;
	}
	void OAMTransfer(uint16_t address) {
		ppu->WriteOAM(address, RAM[address]);
		//if(((address % 4) == 1) || ((address % 4) == 2))
		//	std::cout << "dma transfer, address: " << address << "data: " << (int)RAM[address] << "\n";
	}
};