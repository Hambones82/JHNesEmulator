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

	uint8_t PPU_scroll_x = 0;
	uint8_t PPU_scroll_y = 0;
	
	

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

	bool bg_opaque = false;
	bool sprite_0_hit = false;

	//loopy regs
	uint16_t v = 0;
	uint16_t t = 0;
	uint8_t x = 0;
	bool address_latch = false; //loopy w

	Color GetColor(int in_col, int in_row) {
		int col = in_col + (int)PPU_scroll_x;// +col_adjust;
		int row = in_row + (int)PPU_scroll_y;// +row_adjust;
		bg_opaque = false;
		int tile_x = (col) / 8;
		int tile_y = (row) / 8;
		bool x_increment = 0;
		bool y_increment = 0;
		
		if (tile_x > 31) {
			tile_x -= 32;
			x_increment = 1;
		}
		if (tile_y > 29) {
			tile_y -= 29;
			y_increment = 1;
		}
			
		int tile_id = tile_x + (tile_y * 32);
		
		uint16_t base_nametable_increment;
		base_nametable_increment = (PPUregs.PPUFlags.PPUCTRL.Base_nametable_address << 10);
		
		uint16_t base_nametable_address = 0x2000 + base_nametable_increment;
		uint16_t tile_addr = base_nametable_address + tile_id;
		
		if (x_increment) {
			tile_addr += 0x400;
		}
		if (y_increment) {
			tile_addr += 0x400;
		}
			
		PPUAddr.address = tile_addr;

		uint8_t tile_num = ReadAddr(tile_addr);
		if(tile_addr > 0x3FC0) std::cout << "reading from address that's too high\n"; //this isn't the cause...

		uint8_t block_x = tile_x / 4;
		uint8_t block_y = tile_y / 4;

		uint8_t quad_index_x = (tile_x % 4) / 2;
		uint8_t quad_index_y = (tile_y % 4) / 2;

		uint16_t attr_base = 0x3C0;

		uint16_t attr_addr = base_nametable_address + (x_increment * 0x400) + (y_increment * 0x400) + attr_base + block_x + block_y * 8;

		/*
		uint16_t attr_base = (tile_addr & 0b0000'1100'0000'0000) + 0x3C0;

		uint16_t attr_addr = base_nametable_address + attr_base + block_x + block_y * 8;
		*/
		
		uint8_t attr_byte = ReadAddr(attr_addr); //covers 16 tiles... 4x4 -- probably reading this wrong...

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
		fetch_address &= 0x3FFF; //??
		uint16_t fetch_address_1 = fetch_address + 8;
		fetch_address_1 &= 0x3FFF; //??

		uint8_t plane_1_byte = ReadAddr(fetch_address);
		uint8_t plane_2_byte = ReadAddr(fetch_address_1);

		uint8_t bit_pos = (7 - (col_offset));

		uint8_t lsbit = (plane_1_byte & (1 << bit_pos)) >> bit_pos;
		uint8_t msbit = (plane_2_byte & (1 << bit_pos)) >> bit_pos << 1;

		if(bg) bg_opaque = (lsbit || msbit);
		//if (bg_opaque) std::cout << "bg opaque";

		if (!(lsbit | msbit) && !bg) return 0xFF; //if foreground and 00 index into pallette, return FF meaning transparent

		uint16_t pallette_addr = (bg ? 0x3F00 : 0x3F10) + (attr << 2) + lsbit + msbit;

		uint8_t pallette_index = ReadAddr(pallette_addr);

		return pallette_index;
	}

	int sprite_0_in_current_row = -1;
	int test_ctr = 0;

	int displayntcounter = 0;

public:
	void Tick() {
		//std::cout << "row: " << row << "col: " << col << "\n";
		col++;

		if (col >= max_col) {
			col = 0;
			row++;
			current_row_sprite_counter = 0;
			sprite_0_in_current_row = -1;
			//evaluate sprites for current row
			bool sprite_8x16 = PPUregs.PPUFlags.PPUCTRL.Sprite_size;
			for (int i = 0; i < 64; i++) {
				if (((OAM.sprites[i].y_pos) <= row) && ((OAM.sprites[i].y_pos) > (row - (sprite_8x16?15:8)))) {
					if (i == 0) { sprite_0_in_current_row = current_row_sprite_counter; }
					//std::cout << "sprite 0 in current row " << sprite_0_in_current_row << " row: " << (int)row << "\n";
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
			bool sprite_8x16 = PPUregs.PPUFlags.PPUCTRL.Sprite_size;
			for (int i = 0; i < current_row_sprite_counter; i++) {
				if ((current_row_sprites[i].x_pos > col - 8) &&
					(current_row_sprites[i].x_pos <= col/* << (sprite_8x16 ? 1 : 0)*/)) {
					uint8_t attr = current_row_sprites[i].attributes & 0x03;
					bool bottom_tile = (row - current_row_sprites[i].y_pos) >= 8;
					uint8_t top_tile_num = current_row_sprites[i].tile_index;
					uint8_t tile_num = (sprite_8x16 && bottom_tile)?top_tile_num+1:top_tile_num;

					uint8_t x_offset = ((col - current_row_sprites[i].x_pos) % 8);
					if (current_row_sprites[i].attributes & 0b0100'0000) {
						x_offset = 7 - x_offset;
					}
					uint8_t y_offset = (row - current_row_sprites[i].y_pos) % 8;
					if (current_row_sprites[i].attributes & 0b1000'0000) {
						y_offset = 7 - y_offset;
					}
					uint8_t side = PPUregs.PPUFlags.PPUCTRL.Sprite_size ?
						current_row_sprites[i].tile_index & 1 :
						PPUregs.PPUFlags.PPUCTRL.Sprite_pattern_table_address;
					uint8_t pallette_index = GetPalletteIndex(tile_num, y_offset, x_offset, attr, false, side);
					if (pallette_index != 0xFF) {
						auto color = MasterPallette[pallette_index];
						if (!((current_row_sprites[i].attributes & 0x20) && bg_opaque)) {
							renderOut->SetColor(color.r, color.g, color.b, 0);
							renderOut->DrawPixel(col, row);
						}
						//std::cout << "sprite 0 in current row " << sprite_0_in_current_row << " row: " << (int)i << "\n";
						if (bg_opaque && (sprite_0_in_current_row == i)) {
							//std::cout << "bg_opaque";
							if (sprite_0_hit == false) {
								PPUregs.PPUFlags.PPUSTATUS.sprite_0_hit = 1;
								//std::cout << "sprite 0 hit" << "row: " << row << ", col: " << col << "\n";
							}
							sprite_0_hit = true;
						}
					}
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
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 1;//flag of the 2002 register
			/*
			if (displayntcounter++ == 100) {
				DisplayNT();
				displayntcounter = 0;
			}*/
			
			renderOut->EndFrame();
			/*
			if (sprite_0_hit) {
				//std::cout << "sprite 0 hit\n";
				PPUregs.PPUFlags.PPUSTATUS.sprite_0_hit = 1;
			}*/
			frame++;
		}
		else if ((row == 261) && (col == 0)) {
			//std::cout << "starting frame\n";
			PPUregs.PPUFlags.PPUSTATUS.vblank_started = 0;//flag of the 2002 register
			renderOut->StartFrame();
		}
		else if ((row == 261) && (col == 1)) {
			PPUregs.PPUFlags.PPUSTATUS.sprite_0_hit = 0;
			sprite_0_hit = false;
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

	uint8_t PPUReadBuffer = 0;
	uint8_t ReadReg(uint8_t reg_num) {
		if (reg_num == 7) {
			uint8_t temp_read_buf = PPUReadBuffer;
			PPUReadBuffer = ReadAddr(PPUAddr.address);
			PPUAddr.address += (PPUregs.PPUFlags.PPUCTRL.VRAM_address_increment ? 32 : 1); //increment based on 0x2000
			return temp_read_buf;
		}
		if (reg_num == 4) {
			return OAM.bytes[PPUregs.PPUFlags.OAMADDR];
		}
		if (reg_num == 2) {
			address_latch = false;
			return PPUregs.bytes[2];
		}
		else if (reg_num < 8) {
			return PPUregs.bytes[reg_num];
		}
		else return 0;
	}

	bool reg_write_debug_out = false;

	void WriteReg(uint8_t reg_num, uint8_t value) {
		if (reg_num == 5) {
			if (reg_write_debug_out)
			{
				std::cout << "writing reg 2005 (scroll): " << std::hex << (int)value << "row: " << row << ", col: " << col << "\n";
			}
				
			if (address_latch) {
				PPU_scroll_y = value;
			}
			else {
				PPU_scroll_x = value;
			}
			address_latch = !address_latch;

			x = value & 0x07;
			t |= ((value >> 3) & 0x1F);
		}
		else if (reg_num == 6) {
			
			if (reg_write_debug_out) {
				std::cout << "writing reg 2006 (ppuaddr): " << std::hex << (int)value << "row: " << row << ", col: " << col << "\n";
			}
			
			if (address_latch == false) {
				PPUAddr.bytes[1] = value;
				address_latch = true;
			}
			else {
				PPUAddr.bytes[0] = value;
				PPUregs.PPUFlags.PPUCTRL.Base_nametable_address = (value & 0x0C) >> 10;
				address_latch = false;
			}
		}
		else if (reg_num == 7) {
			if (reg_write_debug_out) {
				std::cout << "writing reg 2007 (ppudata): " << std::hex << (int)value << "row: " << row << ", col: " << col << "\n";
			}
			
			WriteAddr(PPUAddr.address, value);
			PPUAddr.address += (PPUregs.PPUFlags.PPUCTRL.VRAM_address_increment ? 32 : 1); //increment based on 0x2000
			PPUAddr.address = PPUAddr.address % 0x4000;
		}
		else if (reg_num < 8) {
			PPUregs.bytes[reg_num] = value;
		}
		if (reg_num == 0) {
			if (reg_write_debug_out) {
				std::cout << "writing reg 2000: " << std::hex << (int)value << "row: " << row << ", col: " << col << "\n";
			}
			
			t &= 0xF3FF;
			uint16_t update_value = uint16_t(value & 0x03) << 10;
			t |= uint16_t(value & 0x03) << 10;
			/*
			if (PPUregs.PPUFlags.PPUCTRL.Base_nametable_address != 0) {
				std::cout << "base nametable address: " << (int)PPUregs.PPUFlags.PPUCTRL.Base_nametable_address << "\n";
				std::cout << "ppu addr before inputting nametable address: " << std::hex << (int)PPUAddr.address << "\n";
			}
			*/
			/*
			if ((PPUAddr.address) >= 0x2000 && (PPUAddr.address < 0x3F00))//hackiest hack ever
			{
				PPUAddr.address &= 0xF3FF;//??
				PPUAddr.address |= ((uint16_t)(PPUregs.PPUFlags.PPUCTRL.Base_nametable_address) << 10);//??
			}
			*/
			/*
			if (PPUregs.PPUFlags.PPUCTRL.Base_nametable_address != 0) {
				std::cout << "ppu addr after inputting nametable address: " << std::hex << (int)PPUAddr.address << "\n";
			}
			*/
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

	void DisplayNT();
};