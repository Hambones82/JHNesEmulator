#include <stdint.h>

export module APUData;

export enum class EnvelopeFlag {constant_time, envelope};

//https://www.nesdev.org/wiki/APU_Pulse
export struct SquareData {
	bool has_changed = false;
	
	uint8_t duty_cycle = 0;//?
	bool halt_remaining_time = false;
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
	bool has_changed = false;

	bool length_counter_halt;
	uint8_t linear_counter_reload;
	uint16_t timer;
	uint8_t length_counter_load;
};

export struct APUData {
	SquareData square1Data;
	SquareData square2Data;
	TriangleData triangleData;
};