#include <cstdint>
#include <array>
#include <utility>
#include <iostream>
#include <sstream>
#include <string>
#include <bitset>
#include<iomanip>

//i think brk is broken...

import NES_Memory_System;
import NESCPU_state;
import MasterClock;
export module NESCPU;

export class NESCPU {
private:
	using instruction = void (*)(NESCPU*);

	static const uint16_t reset_vector = 0xFFFC;
	static const uint16_t NMI_vector = 0xFFFA;
	static const uint16_t BRK_IRQ_vector = 0xFFFE;
	static const uint16_t NMI_IRQ_vector = 0xFFFA;
	static const uint16_t RESET_vector = 0xFFFC;

	static const uint16_t aModeMask = 0x1c;

	static const uint16_t amode_grpB_indirect = 0;
	static const uint16_t amode_grpB_imm = 0x08;
	static const uint16_t amode_grpB_ind_y = 0x10;

	static const uint16_t amode_zpg = 0x04;
	static const uint16_t amode_absolute = 0x0C;
	static const uint16_t amode_zpg_x = 0x14; //for STX and LDX, this is actually zero page, y
	static const uint16_t amode_abs_y = 0x18;
	static const uint16_t amode_abs_x = 0x1C; //for LDX, abs_y is used

	static const uint16_t instrGrpMask = 0x03;

	//these groups refer to the red, green, blue, and grey instruction groups
	//described at https://www.nesdev.org/wiki/CPU_unofficial_opcodes

	static const uint16_t instGrpA = 0x00;
	static const uint16_t instGrpB = 0x01;
	static const uint16_t instGrpC = 0x02;
	static const uint16_t instGrpD = 0x03;

	static const uint16_t instMask = 0xE0;

	static const uint16_t ORA_mask = 0x00;
	static const uint16_t AND_mask = 0x20;
	static const uint16_t EOR_mask = 0x40;
	static const uint16_t ADC_mask = 0x60;
	static const uint16_t STA_mask = 0x80;
	static const uint16_t LDA_mask = 0xA0;
	static const uint16_t CMP_mask = 0xC0;
	static const uint16_t SBC_mask = 0xE0;

	const static uint8_t flag_carry = 0;
	const static uint8_t flag_zero = 1;
	const static uint8_t flag_interrupt_disable = 2;
	const static uint8_t flag_decimal = 3;
	const static uint8_t flag_overflow = 6;
	const static uint8_t flag_negative = 7;

	enum class addressing_mode {invalid, ind_x, zpg, imm, abs, ind_y, zpg_ind, zpg_y, abs_y, abs_x, relative};
	enum class instruction_group {invalid, A, B, C, D};
	
	int cycle = 0;
	int test_cycle = 0;

NES_Memory_System* memory_System;
MasterClock* masterClock;
struct {
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint16_t PC;
	uint8_t S; // stack pointer
	uint8_t P; // flags
}regs;

bool reset = false;

void tick() {
	cycle+=3;
	test_cycle = (test_cycle + 3) % 341;
	//PPU tick x 3
	masterClock->MasterClockTick();
	masterClock->MasterClockTick();
	masterClock->MasterClockTick();
}
int last_cycle = 0;
void NMI() {
	std::cout << "nmi. pc: " << std::hex << regs.PC << " cycles since nmi : " << cycle - last_cycle << "\n";
	SetFlag(flag_interrupt_disable, 1);
	uint16_t return_addr = regs.PC;
	StackPushWord(return_addr);
	StackPush(regs.P | 0x04);
	regs.PC = memory_System->FetchWord(NMI_IRQ_vector);
	tick();
	
	last_cycle = cycle;
}

static consteval bool MatchesPattern(uint16_t value, uint16_t mask, uint16_t pattern) {
	return ((value & mask) == pattern);
}

template<size_t opcode>
static consteval instruction_group GetInstructionGroup() {
	if (MatchesPattern(opcode, instrGrpMask, instGrpA)) return instruction_group::A;
	if (MatchesPattern(opcode, instrGrpMask, instGrpB)) return instruction_group::B;
	if (MatchesPattern(opcode, instrGrpMask, instGrpC)) return instruction_group::C;
	return instruction_group::D;
}

template<size_t opcode> 
static consteval addressing_mode GetAddressingMode() { //i think, at least initially, this is correct.  
	if (MatchesPattern(opcode, aModeMask, amode_zpg)) { // zero page -- d -- all instructions that match this bit pattern are addressed this way
		return addressing_mode::zpg;
	}
	else if ((MatchesPattern(opcode, aModeMask, amode_absolute) ) 
								&& opcode != 0x4C && opcode != 0x6C) {
		// absolute -- all instructions that match this bit pattern are addressed this way
		return addressing_mode::abs;
	}
	else if (MatchesPattern(opcode, 0x9D, 0x80) || MatchesPattern(opcode, 0x1F, 0x09)) {//immediate mode
		return addressing_mode::imm;
	}
	else if (MatchesPattern(opcode, 0x1F, 0x10)) {//relative
		return addressing_mode::relative;  
	}
	else if (MatchesPattern(opcode, aModeMask, amode_zpg_x) && (opcode != 0x96) && (opcode != 0xB6)) {
		return addressing_mode::zpg_ind;
	}
	else if ((opcode == 0x96) || (opcode == 0xB6)) {
		return addressing_mode::zpg_y;
	}
	else if (MatchesPattern(opcode, aModeMask, amode_abs_x) && (opcode != 0xBE)) {//absolute-x -- exception is LDX, absolute, indexed
		return addressing_mode::abs_x;
	}
	else if (opcode == 0xBE) { // LDX absolute, indexed
		return addressing_mode::abs_y;
	}
	else if (MatchesPattern(opcode, instrGrpMask, instGrpB)) {
		if (MatchesPattern(opcode, aModeMask, amode_grpB_indirect)) { // indirect, X (d,x) aModeMask = 0x1C (0001`1100)
			return addressing_mode::ind_x;
		}
		else if (MatchesPattern(opcode, aModeMask, amode_grpB_ind_y)) {// y offset - indirect
			return addressing_mode::ind_y;
		}
		else if (MatchesPattern(opcode, aModeMask, amode_abs_y)) { // absolute, y
			return addressing_mode::abs_y;
		}
	}
	
	return addressing_mode::invalid;
}

inline void StackPush(uint8_t value) {
	uint16_t target_address = ((uint16_t)regs.S); 
	target_address |= 0x0100;
	WriteByte(target_address, value);
	regs.S--;
}

inline uint8_t StackPull() {
	regs.S++;
	uint16_t target_address = ((uint16_t)regs.S & 0x00FF);
	target_address |= 0x0100; //stack is from 0x0100 to 0x01FF
	uint8_t retval = ReadByte(target_address);
	return retval;
}

inline uint16_t StackPullWord() {
	uint16_t low_bits = StackPull();
	uint16_t high_bits = ((uint16_t)StackPull()) << 8;
	return low_bits | high_bits;
}

inline void StackPushWord(uint16_t value) {
	StackPush((uint8_t)(value >> 8));
	StackPush((uint8_t)(value));
}

static inline uint8_t SetBit(uint8_t value, int bitNum, uint8_t set_or_reset) {
	std::bitset<8> retval {value};
	retval[bitNum] = set_or_reset;
	return (uint8_t)retval.to_ulong();
}

static inline uint8_t GetBit(uint8_t value, int bitNum) {
	std::bitset<8> retval{ value };
	return retval[bitNum];
}

inline void SetFlag(uint8_t flg, uint8_t set_or_reset) {
	regs.P = SetBit(regs.P, flg, set_or_reset);
}

inline uint8_t GetFlag(uint8_t flg) {
	return GetBit(regs.P, flg);
}

inline uint16_t GetByteImmediate() {
	return ReadByte(regs.PC++);
}

inline uint16_t GetWordImmediate() {
	uint16_t retval = ReadWord(regs.PC);
	regs.PC += 2;
	return retval;
}

inline uint8_t ReadByte(uint16_t address) {
	uint8_t data = memory_System->FetchByte(address);
	tick();
	return data;
}

inline uint16_t ReadWord(uint16_t address) {
	return  (uint16_t)ReadByte(address) | (uint16_t)ReadByte(address + 1) << 8;
}

inline void WriteByte(uint16_t address, uint8_t data) {
	if (address == 0x4014) {// OAM
		for (int i = 0; i < 256; i++) {
			tick();//???
			memory_System->OAMTransfer(((uint16_t)data << 8) + i);
			tick();
		}
	}
	else {
		memory_System->StoreByte(address, data);
		tick();
	}
}

inline void WriteWord(uint16_t address, uint16_t data) {
	WriteByte(address, data >> 8);
	WriteByte(address + 1, (uint8_t)(data & 0x00FF));
}

inline void TickOnPageCross(uint16_t base_addr, uint16_t new_addr) {
	if ((base_addr & 0xFF00) != (new_addr & 0xFF00)) {
		tick();
	}
}

template<size_t opcode> inline
void SetAddress(uint16_t* out_address) {
	constexpr addressing_mode addr_mode = GetAddressingMode<opcode>();
	if constexpr (addr_mode == addressing_mode::ind_x) {
		tick();
		uint8_t address = GetByteImmediate() + regs.X;
		uint16_t lo_out = ReadByte(address);
		uint16_t hi_out = ReadByte((uint8_t)(address + 1)) << 8;
		*out_address = hi_out | lo_out;
	}
	else if constexpr (addr_mode == addressing_mode::zpg) { 
		*out_address = GetByteImmediate();
	}
	else if constexpr (addr_mode == addressing_mode::imm) { //address is just pc
		*out_address = regs.PC++;
	}
	else if constexpr (addr_mode == addressing_mode::abs) {
		*out_address = GetWordImmediate();
	}
	else if constexpr (addr_mode == addressing_mode::ind_y) {
		uint8_t zpg_loc = GetByteImmediate();
		uint16_t base_address_lo = ReadByte(zpg_loc);
		uint16_t base_address_hi = ReadByte(0xFF & (zpg_loc + 1)) << 8;
		uint16_t base_address = base_address_lo | base_address_hi;
		uint16_t base_address_final = base_address + regs.Y;
				
		*out_address = base_address_final;
		if constexpr (MatchesPattern(opcode, 0x1F, 0x11)) {
			TickOnPageCross(base_address, base_address_final);
		}
	}
	else if constexpr (addr_mode == addressing_mode::zpg_ind) {
		tick();
		*out_address = 0x00ff & (GetByteImmediate() + regs.X);
	}
	else if constexpr (addr_mode == addressing_mode::zpg_y) {
		tick();
		*out_address = 0x00ff & (GetByteImmediate() + regs.Y);
	}
	else if constexpr (addr_mode == addressing_mode::abs_y) {
		uint16_t immediate = GetWordImmediate();
		*out_address = immediate + regs.Y;
		if constexpr (MatchesPattern(opcode, 0x1F, 0x19) || opcode == 0xBE) {
			TickOnPageCross(immediate, *out_address);
		}
	}
	else if constexpr (addr_mode == addressing_mode::abs_x) {
		uint16_t immediate = GetWordImmediate();
		*out_address = immediate + regs.X;
		if constexpr ((MatchesPattern(opcode, 0x1F, 0x1D) || opcode == 0xBC) && opcode != 0x9D) {
			TickOnPageCross(immediate, *out_address);
		}
		if constexpr (MatchesPattern(opcode, 0x1F, 0x1E) && (opcode != 0xBE)) {
			tick();
		}
	}
	else if constexpr (addr_mode == addressing_mode::relative) {
		
	}
}

inline void Compare(uint8_t a, uint8_t b) {
	SetFlag(flag_zero, a == b);
	SetFlag(flag_carry, a >= b);
	SetFlag(flag_negative, ((uint8_t)(a-b))>>7);
}

inline uint8_t GetCarryFrom16BitResult(uint16_t result) {
	if (result > 0x00FF) return 1;
	else return 0;
}

inline void SetOverflowFrom16BitResult(int16_t result) { 
	if ((int16_t)result < -128 || (int16_t)result > 127) {
		SetFlag(flag_overflow, 1);
	}
	else {
		SetFlag(flag_overflow, 0);
	}
}

inline void SetZero(uint16_t result) {
	if (result == 0) SetFlag(flag_zero, 1);
	else SetFlag(flag_zero, 0);
}

inline void SetNegative(uint16_t result) { 
	if (GetBit(result, 7)) SetFlag(flag_negative, 1);
	else SetFlag(flag_negative, 0);
}

inline void ADC(uint16_t operand1, uint16_t* in_address) {
	uint8_t carry_in = GetFlag(flag_carry);
	uint16_t result = regs.A + operand1 + carry_in;
	uint8_t overflow = (~(regs.A ^ operand1) & (regs.A ^ result) & 0x80) >> 7;
	regs.A = result;
	uint8_t out_carry = GetCarryFrom16BitResult(result);
	SetFlag(flag_carry, out_carry);
	SetFlag(flag_overflow, overflow);
	SetNegative(regs.A);
	SetZero(regs.A);
}

inline uint8_t ASL(uint8_t value) {
	uint16_t result = value << 1;
	uint8_t carry = result >> 8;
	SetFlag(flag_carry, carry);
	SetNegative(result);
	SetZero(result & 0xFF);
	return result;
}

inline uint8_t ROL(uint8_t value) {
	uint8_t carry_temp = GetFlag(flag_carry);
	uint16_t result = value << 1;
	uint8_t carry = result >> 8;
	result = SetBit(result, 0, carry_temp);
	SetFlag(flag_carry, carry);
	SetNegative(result);
	SetZero(result & 0xFF);
	return result;
}

inline uint8_t LSR(uint8_t value) {
	SetFlag(flag_carry, value & 0x01);
	SetFlag(flag_negative, 0);
	SetZero(value >> 1);
	return value >> 1;
}

inline uint8_t ROR(uint8_t value) {
	uint8_t temp_carry = value & 0x01;
	uint8_t retval = value >> 1;
	retval = SetBit(retval, 7, GetFlag(flag_carry));
	SetFlag(flag_carry, temp_carry);
	SetNegative(retval);
	SetZero(retval);
	return retval;
}

template<size_t opcode> inline void PerformOperation(uint16_t *in_address, bool *branch, uint16_t *branch_target) {
	//ORA, AND, EOR, ADC, STA, LDA, CMP, SBC
	if constexpr (MatchesPattern(opcode, instrGrpMask, 0x01)) {
		if constexpr (MatchesPattern(opcode, instMask, ORA_mask)) {
			uint8_t operand = ReadByte(*in_address);
			regs.A |= operand;
			SetNegative(regs.A);
			SetZero(regs.A);
		}
		else if constexpr (MatchesPattern(opcode, instMask, AND_mask)) {
			uint8_t operand = ReadByte(*in_address);
			regs.A &= operand;
			SetNegative(regs.A);
			SetZero(regs.A);
		}
		else if constexpr (MatchesPattern(opcode, instMask, EOR_mask)) {
			uint8_t operand = ReadByte(*in_address);
			regs.A ^= operand;
			SetNegative(regs.A);
			SetZero(regs.A);
		}
		else if constexpr (MatchesPattern(opcode, instMask, ADC_mask)) { //no idea if the flag bits are correct...
			uint8_t operand = ReadByte(*in_address);
			ADC(operand, in_address);
		}
		else if constexpr (MatchesPattern(opcode, instMask, STA_mask)) {
			WriteByte(*in_address, regs.A);
			if constexpr (opcode == 0x9D || opcode == 0x99 || opcode == 0x91) {
				tick();
			}
		}
		else if constexpr (MatchesPattern(opcode, instMask, LDA_mask)) {
			uint8_t operand = ReadByte(*in_address);
			regs.A = operand;
			SetNegative(regs.A);
			SetZero(regs.A);
		}
		else if constexpr (MatchesPattern(opcode, instMask, CMP_mask)) {
			uint8_t operand = ReadByte(*in_address);
			Compare(regs.A, operand);
		}
		else if constexpr (MatchesPattern(opcode, instMask, SBC_mask)) {
			uint8_t operand = ReadByte(*in_address);
			ADC(0xFF ^ operand, in_address);
		}
	}
	else if constexpr (MatchesPattern(opcode, instrGrpMask, 0x02)) { // group c (blue)
		if constexpr (MatchesPattern(opcode, instMask, 0)) { //ASL
			if constexpr (opcode == 0x0A) {
				regs.A = ASL(regs.A);
				tick();
			}
			else {
				uint8_t operand = ReadByte(*in_address);
				WriteByte(*in_address, ASL(operand));
				tick();
			}
		}
		else if constexpr (MatchesPattern(opcode, instMask, 0x20)) { // ROL
			if constexpr (opcode == 0x2A) {
				regs.A = ROL(regs.A);
				tick();
			}
			else {
				uint8_t operand = ReadByte(*in_address);
				WriteByte(*in_address, ROL(operand));
				tick();
			}
		}
		else if constexpr (MatchesPattern(opcode, instMask, 0x40)) { // LSR
			if constexpr (opcode == 0x4A) {
				regs.A = LSR(regs.A);
				tick();
			}
			else {
				uint8_t operand = ReadByte(*in_address);
				WriteByte(*in_address, LSR(operand));
				tick();
			}
		}
		else if constexpr (MatchesPattern(opcode, instMask, 0x60)) { // ROR
			if constexpr (opcode == 0x6A) {
				regs.A = ROR(regs.A);
				tick();
			}
			else {
				uint8_t operand = ReadByte(*in_address);
				WriteByte(*in_address, ROR(operand));
				tick();
			}
		}
		else if constexpr (opcode == 0x86 || opcode == 0x8E || opcode == 0x96) { // STX
			WriteByte(*in_address, regs.X);
		}
		else if constexpr (opcode == 0x8A) { // TXA
			regs.A = regs.X;
			SetNegative(regs.X);
			SetZero(regs.X);
			tick();
		}
		else if constexpr (opcode == 0x9A) { // TXS
			regs.S = regs.X;
			tick();
		}
		else if constexpr (opcode == 0xA2 || opcode == 0xA6 || opcode == 0xAE || opcode == 0xB6 || opcode == 0xBE) { // LDX
			uint8_t operand = ReadByte(*in_address);
			regs.X = operand;
			SetNegative(regs.X);
			SetZero(regs.X);
		}
		else if constexpr (opcode == 0xAA) { // TAX
			regs.X = regs.A;
			SetNegative(regs.X);
			SetZero(regs.X);
			tick();
		}
		else if constexpr (opcode == 0xBA) { // TSX
			regs.X = regs.S;
			SetNegative(regs.X);
			SetZero(regs.X);
			tick();
		}
		else if constexpr (opcode == 0xC6 || opcode == 0xCE || opcode == 0xD6 || opcode == 0xDE) { // DEC
			uint8_t operand = ReadByte(*in_address);
			uint8_t result = operand - 1;
			WriteByte(*in_address, result);
			SetNegative(result);
			SetZero(result);
			tick();
		}
		else if constexpr (opcode == 0xCA) { // DEX
			regs.X--;
			SetNegative(regs.X);
			SetZero(regs.X);
			tick();
		}
		else if constexpr (opcode == 0xF6 || opcode == 0xFE || opcode == 0xE6 || opcode == 0xEE) { // INC
			uint8_t operand = ReadByte(*in_address);
			uint8_t result = operand + 1;
			SetNegative(result);
			SetZero(result);
			WriteByte(*in_address, result);
			tick();
		}
		else {
			tick();
		}
	}
		//above --> all the blue ones
		//below --> all the red ones...
	else if constexpr (opcode == 0) { //BRK
		uint16_t return_addr = regs.PC;
		StackPushWord(return_addr);
		StackPush(regs.P | 0x04); 
		*branch = true;
		*branch_target = memory_System->FetchWord(BRK_IRQ_vector);
		//jump to interrupt vector?
	}
	else if constexpr (opcode == 0x20) { //JSR
		uint16_t return_addr = regs.PC; 
		uint16_t target_low = GetByteImmediate();
		StackPushWord(regs.PC);
		uint16_t target_high = GetByteImmediate();
		uint16_t target = (target_high << 8) | target_low;
		regs.PC = target;
		tick();
	}
	else if constexpr (opcode == 0x40) { //RTI
		uint8_t new_stack_reg = StackPull() & 0xcb;
		uint8_t current_ignored_flags = (0x34 & regs.P);
		regs.P = new_stack_reg | current_ignored_flags;
		regs.PC = StackPullWord();
		tick();
		tick();
	}
	else if constexpr (opcode == 0x60) { //RTS
		regs.PC = StackPullWord() + 1;
		tick();
		tick();
		tick();
	}
	else if constexpr (opcode == 0x84 || opcode == 0x8C || opcode == 0x94) { // STY
		WriteByte(*in_address, regs.Y);
	}
	else if constexpr (opcode == 0x88) { //DEY
		regs.Y--;
		SetNegative(regs.Y);
		SetZero(regs.Y);
		tick();
	}
	else if constexpr (opcode == 0x98) { // TYA
		regs.A = regs.Y;
		SetNegative(regs.Y);
		SetZero(regs.Y);
		tick();
	}
	else if constexpr (opcode == 0xA0 || opcode == 0xA4 || opcode == 0xAC || opcode == 0xB4 || opcode == 0xBC) { //LDY
		uint8_t operand = ReadByte(*in_address);
		regs.Y = operand;
		SetNegative(regs.Y);
		SetZero(regs.Y);
	}
	else if constexpr (opcode == 0xA8) { // TAY
		regs.Y = regs.A;
		SetNegative(regs.Y);
		SetZero(regs.Y);
		tick();
	}
	else if constexpr (opcode == 0xC0 || opcode == 0xC4 || opcode == 0xCC) { //CPY
		uint8_t operand = ReadByte(*in_address);
		Compare(regs.Y, operand);
	}
	else if constexpr (opcode == 0xC8) { // INY
		regs.Y++;
		SetNegative(regs.Y);
		SetZero(regs.Y);
		tick();
	}
	else if constexpr (opcode == 0xE0 || opcode == 0xE4 || opcode == 0xEC) { //CPX
		uint8_t operand = ReadByte(*in_address);
		Compare(regs.X, operand);
	}
	else if constexpr (opcode == 0xE8) { // INX
		regs.X++;
		SetNegative(regs.X);
		SetZero(regs.X);
		tick();
	}
	else if constexpr(MatchesPattern(opcode, 0x1F, 0x18) && (opcode != 0x98) && (opcode != 0xB8)) { // flags instructions, other than TYA
		constexpr uint8_t set_or_reset = (opcode & 0x20) >> 5;
		constexpr uint8_t flag_mask = opcode & 0xC0;
		if constexpr (flag_mask == 0x00) {
			SetFlag(flag_carry, set_or_reset);
		}
		else if constexpr (flag_mask == 0x40) {
			SetFlag(flag_interrupt_disable, set_or_reset);
		}
		else if constexpr (flag_mask == 0xC0) {
			SetFlag(flag_decimal, set_or_reset);
		}
		tick();
	}
	else if constexpr (opcode == 0xB8) { // CLV
		SetFlag(flag_overflow, 0);
		tick();
	}
	else if constexpr ((opcode & 0x1F) == 0x10) { // branches
	
		constexpr uint8_t branch_type = opcode & 0xE0;
		int8_t branch_offset = GetByteImmediate();
		uint16_t old_addr = regs.PC;
		*branch_target = regs.PC + branch_offset;
		
		if constexpr (branch_type == 0) { //BPL
			*branch = !GetFlag(flag_negative);
		}
		else if constexpr (branch_type == 0x20) { //BMI
			*branch = GetFlag(flag_negative);
		}
		else if constexpr (branch_type == 0x40) { //BVC
			*branch = !GetFlag(flag_overflow);
		}
		else if constexpr (branch_type == 0x60) { //BVS
			*branch = GetFlag(flag_overflow);
		}
		else if constexpr (branch_type == 0x80) { //BCC
			*branch = !GetFlag(flag_carry);
		}
		else if constexpr (branch_type == 0xA0) { //BCS
			*branch = GetFlag(flag_carry);
		}
		else if constexpr (branch_type == 0xC0) { //BNE
			*branch = !GetFlag(flag_zero);
		}
		else if constexpr (branch_type == 0xE0) { //BEQ
			*branch = GetFlag(flag_zero);
		}
		if (*branch) {
			TickOnPageCross(old_addr, *branch_target);
		}
	}
	else if constexpr (opcode == 0x4C || opcode == 0x6C) { //unconditional jump
		uint16_t jmp_target = 0;
		if constexpr (opcode == 0x4C) {
			jmp_target = GetWordImmediate();
		}
		else if constexpr (opcode == 0x6C) {
			uint8_t indir_addr_lo = GetByteImmediate();
			uint8_t indir_addr_hi = GetByteImmediate();
			uint16_t branch_target_lo = ReadByte((uint16_t(indir_addr_hi) << 8) + indir_addr_lo);
			uint8_t indir_addr_lo_inc = indir_addr_lo + 1;
			uint16_t branch_target_hi = ReadByte((uint16_t(indir_addr_hi) << 8) + indir_addr_lo_inc);
			jmp_target = (branch_target_hi << 8) + branch_target_lo;
		}
		regs.PC = jmp_target;
	}
	else if constexpr (opcode == 0x2C || opcode == 0x24) { //BIT
		uint8_t operand = ReadByte(*in_address);
		uint8_t result = regs.A & operand;
		SetFlag(flag_zero, result == 0);
		SetFlag(flag_negative, operand >> 7);
		SetFlag(flag_overflow, (operand & 0x40) >> 6);
	}
	else if constexpr (opcode == 0x08) { //PHP
		StackPush(regs.P | 0b0011'0100); //set bit 5 and bit 2
		tick();
	}
	else if constexpr (opcode == 0x28) { //PLP
		uint8_t P_ignore_bits_mask = regs.P & 0b0011'0100;
		uint8_t stack_val_without_ignore_bits = StackPull() & 0b1100'1011;
		regs.P = P_ignore_bits_mask | stack_val_without_ignore_bits;
		tick();
		tick();
	}
	else if constexpr (opcode == 0x48) { //PHA
		StackPush(regs.A);
		tick();
	}
	else if constexpr (opcode == 0x68) { //PLA
		regs.A = StackPull();
		SetNegative(regs.A);
		SetZero(regs.A);
		tick();
		tick();
	}
	else { tick(); }//NOP
}

template<size_t opcode>
static void Instruction(NESCPU *cpu) {
		 uint16_t address;
		 uint16_t branch_target = 0;
		 bool branch = false;
		 cpu->SetAddress<opcode>(&address);
		 cpu->PerformOperation<opcode>(&address, &branch, &branch_target);
		 if (branch) {
			 cpu->regs.PC = branch_target;
			 cpu->tick();
		 }
	}

template<std::size_t...Is>
static consteval std::array<instruction, 256> generateInstructions(std::index_sequence<Is...>) {
		 return { &Instruction<Is>... };
	 }

const std::array<instruction, 256> instructions = generateInstructions(std::make_index_sequence<256>());

public:
NESCPU(NES_Memory_System *in_memory, MasterClock *in_MasterClock) {
		regs.A = 0;
		regs.X = 0;
		regs.Y = 0;
		regs.P = 0x24;
		regs.S = 0xFD;
		regs.PC = 0x0000;
		memory_System = in_memory;
		masterClock = in_MasterClock;
		reset = true;
	}

void AdvanceExecution() {
	if (reset) {
		regs.PC = memory_System->FetchWord(RESET_vector);
		reset = false;
	}
	else if (memory_System->GetNMI()) {
		//std::cout << "nmi\n";
		NMI();
		}
	else {
		instructions[ReadByte(regs.PC++)](this);
	}
	//std::cout << "PC: " << std::hex << (int)regs.PC << "\n";
	}

std::string GetState() {
		std::stringstream ss;
		ss  << "A: "   << std::setw(2) << std::setfill('0') << std::hex  << (int)regs.A
			<< " P: "  << std::setw(2) << std::setfill('0') << std::hex  << (int)regs.P
			<< " PC: " << std::setw(4) << std::setfill('0') << std::hex << (int)regs.PC
			<< " S: "  << std::setw(2) << std::setfill('0') << std::hex << (int)regs.S
			<< " X: "  << std::setw(2) << std::setfill('0') << std::hex  << (int)regs.X
			<< " Y: "  << std::setw(2) << std::setfill('0') << std::hex  << (int)regs.Y
			<< " Cyc: " << std::setw(3) << std::dec << test_cycle;
		return ss.str();
	}

NESCPU_state GetStateObj() {
	NESCPU_state retval;
	retval.A = regs.A;
	retval.P = regs.P;
	retval.PC = regs.PC;
	retval.S = regs.S;
	retval.X = regs.X;
	retval.Y = regs.Y;
	retval.cycle = cycle;
	retval.test_cycle = test_cycle;
	return retval;

}
};