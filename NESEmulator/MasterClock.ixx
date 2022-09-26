export module MasterClock;
import PPU;

export class MasterClock {
private:
	PPU* PPUptr;
public:
	void MasterClockTick() {
		PPUptr->Tick();
	}
	MasterClock(PPU* in_PPU) {
		PPUptr = in_PPU;
	}
};