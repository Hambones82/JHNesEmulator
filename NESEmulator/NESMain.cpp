/*
import NESMain;
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

NESData::NESData()
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

void NESData::AdvanceCPU() {
	cpu.AdvanceExecution();
}
*/