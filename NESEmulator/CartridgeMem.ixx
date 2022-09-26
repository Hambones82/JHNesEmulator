#include <stdint.h>
#include <array>
#include <vector>

export module CartridgeMem;

export const uint16_t bank_size = 0x2000; //8KB


export typedef std::array<uint8_t, bank_size> bank; //bank loaded from a file.  prg_rom are split across 2 of these.  

export class CartridgeMem {
	const static uint16_t max_banks = 32;
	const static uint16_t banks_in_addr_space = UINT16_MAX + 1 / bank_size;

	const static uint16_t base_addr_mask = 0xE000; //top 3 bits...  address space size (64KB) / bank size (8KB) = 8
	const static uint16_t offset_mask = 0x1FFF;
	const static uint8_t address_shift_amount = 13;//conversion between base addr and base num

	std::array<bank, max_banks> banks; //supposed to represented the banks on cartridge...
	//page table
	std::array<uint16_t, banks_in_addr_space> page_table = {0}; //provides base addresses
	//usage: page_table[page_num] --> base addr.

	void SetPageTableEntry(uint16_t page_base_addr, uint8_t bank_num) {
		page_table[GetPageNum(page_base_addr)] = bank_num;
	}
	inline uint8_t GetPageNum(uint16_t address) {
		return (address & base_addr_mask) >> address_shift_amount; //"large pages" are in increments of 16KB (should it be half that?)
		//shifted by 14 to act as a 2 bit index into prg rom banks
	}
	inline uint16_t GetPageBaseAddr(uint8_t page_num) {
		return ((uint16_t)page_num) << address_shift_amount;
	}
	uint8_t GetBankNum(uint16_t address) {
		uint8_t page_num = GetPageNum(address);
		return page_table[page_num];
	}
	inline uint16_t GetOffset(uint16_t address) { //used as an index into the bank
		return address & offset_mask;
	}
public:
	CartridgeMem() {
		for (int i = 0; i < max_banks; i++) {
			for (int j = 0; j < bank_size; j++) {
				banks[i][j] = 0;
			}
		}
	}
	
	void LoadBank(std::vector<uint8_t> &in_Bank, int bank_num) { 
		std::copy_n(in_Bank.begin(), bank_size, banks[bank_num].begin());
	}
	void MapBank(uint16_t new_base_addr, uint8_t bank_num) {
		SetPageTableEntry(new_base_addr, bank_num);
	}

	void WriteROM(uint16_t address, uint8_t data) {
		uint8_t bank_num = GetBankNum(address);
		uint16_t offset = GetOffset(address);
		banks[bank_num][offset] = data;
	}
	uint8_t ReadROM(uint16_t address) {
		uint8_t bank_num = GetBankNum(address);
		uint16_t offset = GetOffset(address);
		return banks[bank_num][offset];
	}


};