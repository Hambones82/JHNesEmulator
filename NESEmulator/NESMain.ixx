/*
module;
#include <thread>
export module NESMain;

import RenderingWindow;
import NES_Memory_System;
import PPU;
import NESCPU;
import NESROMLoader;
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
	NESData();
	void AdvanceCPU();
};
*/