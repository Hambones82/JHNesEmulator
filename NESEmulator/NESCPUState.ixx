module;
#include <stdint.h>

export module NESCPU_state;

export class NESCPU_state {
public:
	uint16_t PC;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t P; //status
	uint8_t S; //stack pointer
	int cycle;
	int test_cycle;

	NESCPU_state() {
		PC = 0;
		A = 0;
		X = 0;
		Y = 0;
		P = 0;
		S = 0;
		cycle = 0;
		test_cycle = 0;
	}

	bool operator== (NESCPU_state& rhs) {
		return (rhs.PC = PC && 
				rhs.A == A && 
				rhs.X == X && 
				rhs.Y == Y && 
				rhs.P == P && 
				rhs.S == S && 
				rhs.test_cycle == test_cycle);
	}
};