module;
#include <thread>
export module NESMain;
import RenderingWindow;
import NES_Memory_System;
import NESCPU;
import NESROMLoader;
import PPU;
import MasterClock;
import PlatformIO;
import NESIO;
import APU;
import AudioDriver;

export class NESData {
private:
	AudioDriver audioDriver;
	APU apu;
	std::thread audio_thread;
	RenderingWindow nesRenderer;
	PPU ppu;
	PlatformIO platformIO;
	NESIO nesIO;
	NES_Memory_System memory;
	MasterClock masterClock;
	NESCPU cpu;
public:
	NESData()
		: apu(&audioDriver),
		nesRenderer(),
		ppu(&nesRenderer),
		platformIO(),
		nesIO(&platformIO),
		memory(&ppu, &nesIO, &apu),
		masterClock(&ppu, &apu),
		cpu(&memory, &masterClock),
		audio_thread(&AudioThread, &audioDriver)
	{
		ppu.SetMemorySystem(&memory);
		NESROMLoader::LoadRomFile(R"(C:\Users\Josh\source\NESEmulator\Roms\SuperMarioBros.nes)", &memory);
	}
	void AdvanceCPU() {
		cpu.AdvanceExecution();
	}
	//update cpu...
};