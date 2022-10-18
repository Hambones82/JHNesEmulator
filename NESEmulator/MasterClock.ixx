export module MasterClock;
import PPU;
import APU;

export class MasterClock {
private:
	PPU* PPUptr;
	APU* apuptr;
public:
	void MasterClockTick() {
		PPUptr->Tick();
		apuptr->Tick();//this is wrong and needs to be fixed...
	}
	MasterClock(PPU* in_PPU, APU* in_APU) {
		PPUptr = in_PPU;
		apuptr = in_APU;
	}
};