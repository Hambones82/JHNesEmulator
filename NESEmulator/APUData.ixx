#include <stdint.h>

export module APUData;

export enum class EnvelopeFlag {constant_time, envelope};

//https://www.nesdev.org/wiki/APU_Pulse
export struct SquareData {
	uint8_t duty_cycle = 0;//?
	bool length_counter_halt = false;
	uint8_t length_counter = 0; //duration of the note
	uint16_t timer = 0; //sets freq.  period is t + 1 APU cycles
	EnvelopeFlag envelope_flag = EnvelopeFlag::constant_time;
	uint8_t envelope_period = 0;

	//sweep
	bool sweep_enabled = false;
	uint8_t sweep_period = 0;
	bool negate = false;
	uint8_t shift_counter = 0;
};

export struct TriangleData {
	bool length_counter_halt;
	uint8_t linear_counter;
	uint8_t linear_counter_reload_value;
	uint16_t timer;
	uint8_t length_counter;

	bool linear_counter_reload_flag = false;
	
};

export struct NoiseData {
	uint16_t shift_reg = 1;
	bool length_counter_halt = false;
	EnvelopeFlag envelopeFlag = EnvelopeFlag::constant_time;
	uint16_t timer = 1;
	uint8_t noise_mode = 0;
	uint8_t envelope_period = 0;
	uint8_t length_counter_load = 0;
	bool envelope_restart = false;
};

export enum class frameCounterMode {mode_4_step, mode_5_step};

export struct APUData {
	frameCounterMode mode = frameCounterMode::mode_4_step;
	float APUcycles = 0;
	SquareData square1Data;
	SquareData square2Data;
	TriangleData triangleData;
	NoiseData noiseData;
};