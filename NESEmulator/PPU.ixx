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
	std::array<uint8_t, 32> pallette;
	struct Sprite {
		uint8_t y_pos;
		uint8_t tile_index;
		uint8_t attributes;
		uint8_t x_pos;
	};
	
	union {
		Sprite sprites[64];
		uint8_t bytes[256];
	}OAM;

	Sprite current_row_sprites[8];
	uint8_t current_row_sprite_counter;

	Color GetColor(int col, int row) {
		
		int tile_x = col / 8;
		int tile_y = row / 8;
		int tile_id = tile_x + (tile_y * 32);
		//if (PPUregs.PPUFlags.PPUCTRL.Base_nametable_address != 0) std::cout << "base nt addr != 0\n";
		uint16_t base_nametable_address = 0x2000 + (PPUregs.PPUFlags.PPUCTRL.Base_nametable_address << 10);
		uint16_t tile_addr = base_nametable_address + tile_id;
		uint8_t tile_num = ReadAddr(tile_addr);

		uint8_t block_x = tile_x / 4;
		uint8_t block_y = tile_y / 4;

		uint8_t quad_index_x = (tile_x % 4) / 2;
		uint8_t quad_index_y = (tile_y % 4) / 2;

		uint16_t attr_addr = base_nametable_address + 0x3C0 + block_x + block_y * 8;
		uint8_t attr_byte = ReadAddr(attr_addr); //covers 16 tiles... 4x4
		uint8_t attr_byte_index = (quad_index_x % 2) + (quad_index_y % 2) * 2;
		uint8_t attr_byte_mask = 0xC0 >> ((3-attr_byte_index)*2);

		uint8_t attr = (attr_byte_mask & attr_byte) >> ((attr_byte_index)*2); //portion of lookup to pallette ram
		
		uint8_t pallette_index = GetPalletteIndex(tile_num, row%8, col%8, attr, true, (PPUregs.PPUFlags.PPUCTRL.Background_pattern_table_address));
		return MasterPallette[pallette_index];
	}
	
	uint8_t GetPalletteIndex(uint8_t tile_num, uint8_t row_offset, uint8_t col_offset, uint8_t attr, bool bg, uint8_t side_select) {
		uint8_t tile_num_row = tile_num / 16;
		uint8_t tile_num_col = tile_num % 16;

		uint16_t tile_side = (uint16_t)(side_select) << 12;
		uint16_t fetch_address = (tile_num_row << 8) + (tile_num_col << 4) + (row_offset) + tile_side;
		uint16_t fetch_address_1 = fetch_address + 8;

		uint8_t plane_1_byte = ReadAddr(fetch_address);
		uint8_t plane_2_byte = ReadAddr(fetch_address_1);

		uint8_t bit_pos = (7 - (col_offset));

		uint8_t lsbit = (plane_1_byte & (1 << bit_pos)) >> bit_pos;
		uint8_t msbit = (plane_2_byte & (1 << bit_pos)) >> bit_pos << 1;

		uint16_t pallette_addr = (bg?0x3F00:0x3F10) + (attr << 2) + lsbit + msbit;
		
		uint8_t pallette_index = ReadAddr(pallette_addr);
		
		return pallette_index;
	}

	int test_ctr = 0;
public:
	void Tick() {
		//std::cout << "row: " << row << "col: " << col << "\n";
		col++;
		
		if (col >= max_col) {
			col = 0;
			row++;
			current_row_sprite_counter = 0;
			//evaluate sprites for current row
			for (int i = 0; i < 64; i++) {
				if (((OAM.sprites[i].y_pos) <= row) && ((OAM.sprites[i].y_pos) > (row - 8))) {
					current_row_sprites[current_row_sprite_counter++] = OAM.sprites[i];
					//std::cout << "sprite " << current_row_sprite_counter-1 << "tile num: " 
					//	<< std::hex << (int)(current_row_sprites[i].tile_index) << "\n";
					if (current_row_sprite_counter >= 8) break;
				}
			}
			
			//for (int i = 0; i < current_row_sprite_counter; i++) {
			//	std::cout << "sprite, row: " << row << "col: " << (int)(current_row_sprites[i].x_pos) << "\n";
			//}
		}
		if (row >= max_row) {
			row = 0;
		}
		if ((col < max_draw_col) && row < max_draw_row) {
			//std::cout << "drawing pixel";
			if (PPUregs.PPUFlags.MASK.Show_background) {
				auto color = GetColor(col, row);
				renderOut->SetColor(color.r, color.g, color.b, 0);
				renderOut->DrawPixel(col, row);
			}
			for (int i = 0; i < current_row_sprite_counter; i++) {
				if ((current_row_sprites[i].x_pos > col-8) && (current_row_sprites[i].x_pos <= col )) {
					uint8_t attr = current_row_sprites[i].attributes & 0x03;
					//if(attr != 0) { //actually... not attr but ... index...
						uint8_t tile_num = (current_row_sprites[i].tile_index); //bank? 0 or 1
						uint8_t x_offset = (col - current_row_sprites[i].x_pos) % 8;
						uint8_t y_offset = (row - current_row_sprites[i].y_pos) % 8;
						uint8_t pallette_index = GetPalletteIndex(tile_num, y_offset, x_offset, attr, false, 0);
						auto color = MasterPallette[pallette_index];
						renderOut->SetColor(color.r, color.g, color.b, 0);
						renderOut->DrawPixel(col, row);
					//}
				}
			}
			//evaluate sprite, etc...
		}
		else if (row == 241 && col == 1) {
			//std::cout << "ending frame\n";
			/*
			for (int i = 0; i < 64; i++) {
				std::cout << "oam entry " << i << " x: " << (int)OAM.sprites[i].x_pos;
				std::cout << ", y: " << (int)OAM.sprites[i].y_pos;
				std::cout << ", attr: " << (int)OAM.sprites[i].attributes;
				std::cout << ", index: " << std::hex << (int)OAM.sprites[i].tile_index << "\n";
			}*/
			std::cout << std::flush;
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 1;//flag of the 2002 register
			renderOut->EndFrame();
			frame++;
		}
		else if ((row == 261) && (col == 0)) {
			//std::cout << "starting frame\n";
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
		uint8_t retval = (PPUregs.PPUFlags.PPUSTATUS.vblank_started && PPUregs.PPUFlags.PPUCTRL.Generate_NMI);
		if (retval) {
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 0;
		}
		return retval;
	}

	void WriteOAM(uint8_t address, uint8_t data) {
		OAM.bytes[address] = data;
		//std::cout << "OAM transfering: " << (int)address << ", " << (int)data << "\n";
	}
};