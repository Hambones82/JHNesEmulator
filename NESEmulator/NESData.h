#pragma once
#include <thread>
#include "IMGuiNES.h"

import IMGuiRenderingWindow;
import NES_Memory_System;
import PPU;
import NESCPU;
import NESROMLoader;
import MasterClock;
import PlatformIO;
import NESIO;
import APU;
import AudioDriver;

class NESData {
private:
	AudioDriver audioDriver;
	APU apu;
	std::thread audio_thread;
	PPU ppu;
	PlatformIO platformIO;
	NESIO nesIO;
	NES_Memory_System memory;
	MasterClock masterClock;
	NESCPU cpu;
	IMGuiNES NESGui;
	IMGuiRenderingWindow nesRenderer;
public:
	NESData() : apu(&audioDriver),
		ppu(&nesRenderer),
		platformIO(),
		nesIO(&platformIO),
		memory(&ppu, &nesIO, &apu),
		masterClock(&ppu, &apu),
		cpu(&memory, &masterClock),
		audio_thread(&AudioThread, &audioDriver),
		nesRenderer(&NESGui),
		NESGui(audioDriver, apu, cpu)
	{

		ppu.SetMemorySystem(&memory);
		NESROMLoader::LoadRomFile(R"(C:\Users\Josh\source\NESEmulator\Roms\SuperMarioBros.nes)", &memory);
	}
	void AdvanceCPU() {
		cpu.AdvanceExecution();
	}
};