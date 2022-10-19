#include <stdint.h>

export module MasterClock;
import PPU;
import APU;

export class MasterClock {
private:
	PPU* PPUptr;
	APU* apuptr;
	uint8_t apu_clock_div = 0;
public:
	void MasterClockTick() {
		PPUptr->Tick();
		apu_clock_div++;
		if (apu_clock_div == 6) {
			apu_clock_div = 0;
			apuptr->Tick();//is this right?
		}
		
	}
	MasterClock(PPU* in_PPU, APU* in_APU) {
		PPUptr = in_PPU;
		apuptr = in_APU;
	}
};