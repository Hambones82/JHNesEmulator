#include <cstdint>
#include <array>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include<iomanip>

import PPU;
import CartridgeMem;
export module NES_Memory_System;

const uint16_t banks_in_addr_space = UINT16_MAX +1 / bank_size;
const uint16_t max_banks = 32;

const uint16_t base_addr_mask = 0xE000; //top 3 bits...  address space size (64KB) / bank size (8KB) = 8
const uint16_t offset_mask = 0x1FFF;
const uint8_t address_shift_amount = 13;//conversion between base addr and base num

export typedef std::array<uint8_t, bank_size> bank; //bank loaded from a file.  prg_rom are split across 2 of these.

//module scope -- actually, i have no idea what the scope of these declarations is.
CartridgeMem PRGROM;
CartridgeMem CHRROM;

export class NES_Memory_System
{
private:
	PPU* ppu;
	std::array<uint8_t, 65536> RAM = { 0 };
	std::array<uint8_t, 1024> NameTable1 = { 0 };
	std::array<uint8_t, 1024> NameTable2 = { 0 };
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
		else {
			return PRGROM.ReadROM(address);
		}
	}

	uint8_t ReadRawPPUAddr(uint16_t address) {
		if ((address >= 0) && (address < 0x2000)) {
			return CHRROM.ReadROM(address);
		}
		else if ((address >= 0x2000) && (address < 0x3000)) {
			uint16_t array_address = address & 0x03FF;
			if ((address & 0x2000) > 0) {
				return NameTable1[array_address];
			}
			else {
				return NameTable2[array_address];
			}
		}
		else if ((address >= 0x3F00) && (address < 0x4000)) {
			return PPU_Pallette_RAM[(address & 0x1F)];
		}
		else return 0;
	}

	void WriteRawPPUAddr(uint16_t address, uint8_t value) {
		if ((address >= 0x2000) && (address < 0x3000)) {
			//write to vram
			//donkey kong has horizontal mirroring
			//2000 = 2400 -- nt1
			//2800 = 2c00 -- nt2
			//array address = address & 0x03FF
			//this is wrong...
			uint16_t array_address = address & 0x03FF;
			if ((address & 0x2000) > 0) {
				NameTable1[array_address] = value;
			}
			else {
				NameTable2[array_address] = value;
			}
		}
		else if ((address >= 0x3F00) && (address < 0x4000)) {
			PPU_Pallette_RAM[(address & 0x1F)] = value;
		}
	}

public:
	//ppu map, ppu fetch functions
	//maybe we could start off with a pattern rom viewer.  that'd probably 
	
	NES_Memory_System(PPU* in_PPU) {
		ppu = in_PPU;
	}

	bool GetNMI() {
		return ppu->GetNMI();
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
};