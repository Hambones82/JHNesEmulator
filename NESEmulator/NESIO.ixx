module;
#include <stdint.h>
export module NESIO;
import PlatformIO;

export enum class IOReg {reg4016, reg4017};

export class NESIO {
private:
	PlatformIO* platformIO;
	//latches all buttons of controller when raised high from low
	//we don't really need to check for low to high, just high to low, which will cause the buttons to latch current input state
	bool controller_1_latch = false;
	//0 = A, 1 = B, 2 = Select, 3 = Start, 4 = Up, 5 = Down, 6 = Left, 7 = Right
	bool controller_1_buttons[8] = { false };
	uint8_t current_button = 0; //resets upon switch of latch from high to low
	void Latch_all_values_1() {
		controller_1_buttons[0] = platformIO->GetA();
		controller_1_buttons[1] = platformIO->GetB();
		controller_1_buttons[2] = platformIO->GetSelect();
		controller_1_buttons[3] = platformIO->GetStart();
		controller_1_buttons[4] = platformIO->GetUp();
		controller_1_buttons[5] = platformIO->GetDown();
		controller_1_buttons[6] = platformIO->GetLeft();
		controller_1_buttons[7] = platformIO->GetRight();
	}
public:
	void writeIOReg(IOReg reg, uint8_t value) {
		if(reg == IOReg::reg4016) {
			if (controller_1_latch == false) {
				controller_1_latch = (value & 1);
			}
			else if ((controller_1_latch == true) && ((value & 1) == 0)) {
				current_button = 0;
				controller_1_latch = false;
				Latch_all_values_1();
			}
		}
		
	}
	uint8_t readIOReg(IOReg reg) {
		if (reg == IOReg::reg4016) {
			if (controller_1_latch == false) {
				return controller_1_buttons[(current_button++ % 8)];
			}
		}
	}

	NESIO(PlatformIO* in_platformIO) {
		platformIO = in_platformIO;
	}
};