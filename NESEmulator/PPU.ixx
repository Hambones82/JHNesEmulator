#include <stdint.h>
#include <iostream>
#include <array>
class NES_Memory_System;

import RenderingWindow;
export module PPU;

export class PPU {
private:
	NES_Memory_System* memory_system;
	RenderingWindow* renderOut;
	int row = 0;
	int col = 0;
	int frame = 0;
	const int max_row = 341;
	const int max_col = 262;
	const int max_draw_row = 240;
	const int max_draw_col = 256;
	bool nmi_occurred = false;
	union {
		uint8_t bytes[8];
		struct {	//the bit values are flipped.
			struct {
				uint8_t Base_nametable_address : 2;
				uint8_t VRAM_address_increment : 1;
				uint8_t Sprite_pattern_table_address : 1;
				uint8_t Background_pattern_table_address : 1;
				uint8_t Sprite_size : 1;
				uint8_t Master_slave_select : 1;
				uint8_t Generate_NMI : 1;
			}PPUCTRL; //$2000
			struct {
				uint8_t Greyscale : 1;
				uint8_t Show_bcg_left : 1;
				uint8_t Show_sprites_left : 1;
				uint8_t Show_background : 1;
				uint8_t Show_sprites : 1;
				uint8_t Emphasize_red : 1;
				uint8_t Emphasize_green : 1;
				uint8_t Emphasize_blue : 1;
			}MASK; //$2001
			struct {
				uint8_t PPU_open_bus : 5;
				uint8_t sprite_overflow : 1;
				uint8_t sprite_0_hit : 1;
				uint8_t vblank_started : 1;
			}PPUSTATUS; //$2002
			uint8_t OAMADDR; //$2003
			uint8_t	OAMDATA; //$2004
			uint8_t PPUSCROLL; //$2005
			uint8_t PPUADDR; //$2006
			uint8_t PPUDATA; //$2007
		}PPUFlags;
	}PPUregs;

	


	uint8_t ReadAddr(uint16_t address);
	void WriteAddr(uint16_t address, uint8_t value);

	union {
		uint16_t address = 0;
		uint8_t bytes[2];
	}PPUAddr;

	bool address_latch = false;

	struct Color {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		Color(uint8_t in_r, uint8_t in_g, uint8_t in_b) {
			r = in_r; 
			g = in_g;
			b = in_b;
		}
		Color() {}
	};

	static const std::array<Color, 64> MasterPallette;

	Color GetColor(int col, int row) {
		
		//fetch identifier for pattern byte
		//fetch pattern byte
		//fetch pallette id from nametable
		//fetch color num from nametable
		//fetch color from master pallette
		//return color
		//return retval;
		int tile_x = col / 8;
		int tile_y = row / 8;
		int tile_id = tile_x + (tile_y * 32);

		uint16_t tile_addr = 0x2000 + tile_id;
		uint8_t tile_num = ReadAddr(tile_addr);

		//std::cout << "tile num: " << (int)tile_num << "\n";

		uint8_t tile_num_row = tile_num / 16;
		uint8_t tile_num_col = tile_num % 16;

		uint16_t tile_side = (uint16_t)(PPUregs.PPUFlags.PPUCTRL.Background_pattern_table_address) << 12;
		uint16_t fetch_address = (tile_num_row << 8) + (tile_num_col << 4) + (row % 8) + tile_side;
		uint16_t fetch_address_1 = fetch_address + 8;

		uint8_t plane_1_byte = ReadAddr(fetch_address);
		uint8_t plane_2_byte = ReadAddr(fetch_address_1);
		uint8_t ored_byte = plane_1_byte | plane_2_byte;

		//std::cout << "ored byte: " << std::dec << (int)ored_byte << "\n";

		uint8_t color_y_n = ored_byte & (1 << (8 - (col % 8)));
		
		if (color_y_n == 0) {
			return Color(0, 0, 0);
		}
		else {
			//std::cout << "color is not black" << test_ctr++ << "\n";
			return Color(255, 255, 255);
		}

		//return MasterPallette[tile_num % 64];
		
	}
	int test_ctr = 0;
public:
	void Tick() {
		//std::cout << "row: " << row << "col: " << col << "\n";
		col++;
		if (col >= max_col) {
			col = 0;
			row++;
			if (row >= max_row) {
				row = 0;
			}
		}
		else if ((col < max_draw_col) && row < max_draw_row) {
			auto color = GetColor(col, row);
			renderOut->SetColor(color.r, color.g, color.b, 0);
			renderOut->DrawPixel(col, row);
		}
		else if (row == 241 && col == 1) {
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 1;//flag of the 2002 register
			renderOut->EndFrame();
			frame++;
		}
		else if ((row == 261) && (col == 0)) {
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 0;//flag of the 2002 register
			renderOut->StartFrame();
		}
	}
	void GetColor(uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) {

	}
	PPU(RenderingWindow *in_renderOut) {
		renderOut = in_renderOut;
	}
	void SetMemorySystem(NES_Memory_System* in_memory_system) {
		memory_system = in_memory_system;
	}

	uint8_t ReadReg(uint8_t reg_num) {
		if (reg_num < 8) {
			return PPUregs.bytes[reg_num];
		}
		if (reg_num == 7) {
			address_latch = false;
		}
		else return 0;
	}

	void WriteReg(uint8_t reg_num, uint8_t value) {
		if (reg_num == 6) {
			//std::cout << "writing reg address: " << (int)value << "\n";
			if (address_latch == false) {
				PPUAddr.bytes[1] = value;
				address_latch = true;
			}
			else {
				PPUAddr.bytes[0] = value;
				address_latch = false;
			}
		}
		else if (reg_num == 7) {
			WriteAddr(PPUAddr.address, value);
			PPUAddr.address += (PPUregs.PPUFlags.PPUCTRL.VRAM_address_increment ? 32 : 1); //increment based on 0x2000
			PPUAddr.address = PPUAddr.address % 0x4000;
		}
		else if (reg_num < 8) {
			PPUregs.bytes[reg_num] = value;
		}
	}

	bool GetNMI() {
		uint8_t retval = PPUregs.PPUFlags.PPUSTATUS.vblank_started;
		if (retval) {
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 0;
		}
		return retval;
	}
};