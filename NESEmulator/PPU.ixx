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
		int x_index = (col / 4)%8;
		int y_index = (col / 4) & 0x78;
		int index = (x_index + y_index);
		return MasterPallette[index];
		
	}

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
			if (address_latch == false) {
				PPUAddr.bytes[0] = value;
				address_latch = true;
			}
			else {
				PPUAddr.bytes[1] = value;
			}
		}
		else if (reg_num == 7) {
			WriteAddr(PPUAddr.address, value);
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