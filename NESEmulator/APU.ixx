#include <stdint.h>
#include <iostream>
#include <array>
import APUData;
import AudioDriver;

export module APU;




//when we are playing, just output a play note to audio driver
//duty cycle can just be update duty cycle...
//halt remaining time can be handled internally
//envelope can be handled with set volume commands
//sweep should be handled internally by setting freq on the audio driver
//set timer --...  i think this should just set freq
//length should be handled internally..

class EnvelopeUnit {
private:
	Instrument instrument;
	uint8_t timer; //when clocked, if 0, generates output clock and reloads period P
	uint8_t period;
	bool start_flag;
	bool loop_flag;
	uint8_t decay_level_counter;
public:
	EnvelopeUnit(Instrument in_instrument) {
		instrument = in_instrument;
	}
	//returns true if a new decay level is produced, false otherwise
	bool Tick() {
		bool retval = false;
		if (start_flag) {
			start_flag = false;
			decay_level_counter = 15;
			timer = period;
			retval = true;
		}
		else if (timer == 0) {
			timer = period;
			if (decay_level_counter == 0) {
				if (loop_flag) {
					decay_level_counter = 15;
					retval = true;
				}
			}
			else {
				decay_level_counter--;
				retval = true;
			}

		}
		else {
			timer--;
		}
		return retval;
	}
	void SetPeriod(uint8_t in_period) {
		period = in_period;
		start_flag = true;
	}
	void SetLoopFlag(bool in_loop_flag) {
		loop_flag = in_loop_flag;
	}
	uint8_t GetDecayLevel() {
		return decay_level_counter;
	}
	float GetDecayLevelFloat() {
		return (float)decay_level_counter / 15.0;
	}
};

export class APU {
private:
	APUData apuData;
	AudioDriver* audioDriver;
	EnvelopeUnit square1Envelope{Instrument::square1};
	EnvelopeUnit square2Envelope{ Instrument::square2 };

	bool reset_4017_triggered = false;
	uint8_t reset_4017_counter = 0;


	std::array<uint8_t, 0x20> length_lut{
			10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
	};
	uint8_t LengthValueLookup(uint8_t counter_value) {
		return length_lut[counter_value];
	}

	void SetDriverFreq(uint16_t timer, Instrument instrument) {
		float new_freq = 1789773. / (16 * (timer + 1));
		if (instrument == Instrument::triangle) {
			new_freq /= 2.0;
		}
		if ((new_freq >= 20) && (new_freq < 14000)) {
			audioDriver->DoVoiceCommand(new_freq, VoiceOp::start, instrument);
		}
		else {
			audioDriver->DoVoiceCommand(0, VoiceOp::stop, instrument);
		}
	}
	void SetDriverVolume(float vol, Instrument instrument) {
		audioDriver->DoVolumeCommand(vol, instrument);
	}

	void StopDriver(Instrument instrument) {
		audioDriver->DoVoiceCommand(1, VoiceOp::stop, instrument);
	}
	void SetSquareData_0_reg(SquareData& inSquareData, uint8_t value, Instrument instrument) {
		inSquareData.duty_cycle = value & 0b1100'0000 >> 6;
		inSquareData.length_counter_halt = value & 0b0010'0000 >> 5;
		inSquareData.envelope_flag = ((value & 0b0001'0000) >> 4) ? EnvelopeFlag::constant_time : EnvelopeFlag::envelope;
		inSquareData.envelope_period = value & 0x0F;
		
		if (inSquareData.envelope_flag == EnvelopeFlag::constant_time) {
			SetDriverVolume((float)(inSquareData.envelope_period) / 15., instrument);
		}
	}

	void SetSquareData_1_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.sweep_enabled = (value & 0b1000'0000) >> 7;
		inSquareData.sweep_period = (value & 0b0111'0000) >> 4;
		inSquareData.negate = (value & 0b0000'1000) >> 3;
		inSquareData.shift_counter = value & 0x07;
	}

	void SetSquareData_2_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.timer &= 0xFF00;
		inSquareData.timer |= value;
	}

	void SetSquareData_3_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.length_counter = LengthValueLookup((value & 0b1111'1000) >> 3);
		inSquareData.timer &= 0x00FF;
		inSquareData.timer |= (value & 0b0000'0111) << 8;
	}
	void SetTriData_8_reg(TriangleData& inTriData, uint8_t value) {
		inTriData.length_counter_halt = (value & 0x80) >> 7;
		inTriData.linear_counter_reload_value = value & 0x7F;
		inTriData.linear_counter_reload_flag = true;
		if ((inTriData.linear_counter_reload_value == 0) && (inTriData.length_counter_halt == 1)) {
			StopDriver(Instrument::triangle);
		}

	}
	void SetTriData_A_reg(TriangleData& inTriData, uint8_t value) {
		inTriData.timer &= 0xFF00;
		inTriData.timer |= value;
		inTriData.linear_counter_reload_flag = true;
	}
	void SetTriData_B_reg(TriangleData& inTriData, uint8_t value) {
		inTriData.timer &= 0x00FF;
		inTriData.timer |= ((uint16_t)value & 0x07) << 8;
		inTriData.length_counter = LengthValueLookup((value & 0xF8) >> 3);
		inTriData.linear_counter_reload_flag = true;
	}
	void SetNoise_C_reg(NoiseData& inNoiseData, uint8_t value) {
		inNoiseData.length_counter_halt = (value & 0b0010'0000) >> 5;
		inNoiseData.envelopeFlag = (value & 0b0001'0000) ? EnvelopeFlag::constant_time : EnvelopeFlag::envelope;
		inNoiseData.envelope_period = value & 0x0F;
	}
	void SetNoise_E_reg(NoiseData& inNoiseData, uint8_t value) {
		inNoiseData.noise_mode = (value & 0x80) >> 7;
		std::array<uint16_t, 0x10> period_lut
		{4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};
		inNoiseData.timer = period_lut[(value & 0x0F)];
	}
	void SetNoise_F_reg(NoiseData& inNoiseData, uint8_t value) {
		inNoiseData.length_counter_load = (value & 0xF8) >> 3;
		inNoiseData.envelope_restart = true;
	}

	void SetStatus(uint8_t value) {
		audioDriver->SetTriangleEnabled(value & 0b0000'0100);
		audioDriver->SetSquare2Enabled(value & 0b0000'0010);
		audioDriver->SetSquare1Enabled(value & 0b0000'0001);

	}

	void ClockLengthCounters() {
		if (!apuData.square1Data.length_counter_halt) {
			if (apuData.square1Data.length_counter > 0) {
				apuData.square1Data.length_counter--;
				if (apuData.square1Data.length_counter == 0) {
					StopDriver(Instrument::square1);
				}
			}
		}
		if (!apuData.square2Data.length_counter_halt) {
			if (apuData.square2Data.length_counter > 0) {
				apuData.square2Data.length_counter--;
				if (apuData.square2Data.length_counter == 0) {
					StopDriver(Instrument::square2);
				}
			}
		}
		if (!apuData.triangleData.length_counter_halt) {
			if (apuData.triangleData.length_counter > 0) {
				apuData.triangleData.length_counter--;
				if (apuData.triangleData.length_counter == 0) { //is this correct?
					StopDriver(Instrument::triangle);
				}
			}
		}
	}
	void ClockEnvTriLin() {
		if (square1Envelope.Tick() && (apuData.square1Data.envelope_flag == EnvelopeFlag::envelope)) {
			audioDriver->DoVolumeCommand(square1Envelope.GetDecayLevelFloat(), Instrument::square1);
		}
		if (square2Envelope.Tick() && (apuData.square2Data.envelope_flag == EnvelopeFlag::envelope)) {
			audioDriver->DoVolumeCommand(square2Envelope.GetDecayLevelFloat(), Instrument::square2);
		}
		
		bool can_silence = apuData.triangleData.linear_counter == 1;
		if (apuData.triangleData.linear_counter_reload_flag) {
			apuData.triangleData.linear_counter = apuData.triangleData.linear_counter_reload_value;
		}
		else if(apuData.triangleData.linear_counter!= 0) {
			apuData.triangleData.linear_counter--;
			//std::cout << "triangle linear counter: " << (int)(apuData.triangleData.linear_counter) << "\n";
		}

		if ((apuData.triangleData.linear_counter == 0) && can_silence) {
			StopDriver(Instrument::triangle);
			//std::cout << "linear ctr silencing tri\n";
		}

		if (!apuData.triangleData.length_counter_halt) {
			apuData.triangleData.linear_counter_reload_flag = false;
		}

	}
public:
	//for now, just try setting frequency, have the audio driver play samples based on the set freq.
	APU(AudioDriver *in_audioDriver) {
		audioDriver = in_audioDriver;
	}
	void Tick() {
		//update apuData based on timing
		apuData.APUcycles += .5;
		switch (apuData.mode) {
		case frameCounterMode::mode_4_step:
			if ((apuData.APUcycles == 3728.5) || (apuData.APUcycles == 7456.5)
				|| (apuData.APUcycles == 11185.5) || (apuData.APUcycles == 14914.5)) {
				ClockEnvTriLin();
			}
			if ((apuData.APUcycles == 7456.5) || (apuData.APUcycles == 14914.5)) {
				ClockLengthCounters();
			}
			if ((apuData.APUcycles == 14915)) {
				apuData.APUcycles = 0;
			}
			break;
		case frameCounterMode::mode_5_step:
			if ((apuData.APUcycles == 3728.5) || (apuData.APUcycles == 7456.5)
				|| (apuData.APUcycles == 11185.5) || (apuData.APUcycles == 18640.5)) {
				ClockEnvTriLin();
			}
			if ((apuData.APUcycles == 7456.5) || (apuData.APUcycles == 18640.5)) {
				ClockLengthCounters();
			}
			if ((apuData.APUcycles == 18641)) {
				apuData.APUcycles = 0;
			}
			break;
		}
		if (reset_4017_triggered) {
			reset_4017_counter--;
			if (reset_4017_counter == 0) {
				reset_4017_triggered = false;
				apuData.APUcycles = 0;
				ClockLengthCounters();
				ClockEnvTriLin();
			}
		}
	}
	//the apu has to be clocked...
	//maybe we need a clock method?  so clock this, it'll take care of all the modifications such as sweep
	//something else, not sure...
	//yeah, clock the cpu, advance state, etc...  only thing is to just not implement the sequencer
	void WriteReg(uint16_t regNum, uint8_t val) {
		if (!(regNum >= 0x4000 && regNum <= 0x4013) && regNum != 0x4015 && regNum != 0x4017) {
			std::cout << "error: writing to a register that isn't handled by the APU\n";
		}
		Instrument instrument_affected;
		switch (regNum) {
		case 0x4000:
			SetSquareData_0_reg(apuData.square1Data, val, Instrument::square1);
			//std::cout <<std::hex<< "writing to 4000 (duty cycle, lenght halt, volume): " << (int)val << "\n";
			square1Envelope.SetPeriod(apuData.square1Data.envelope_period);
			square1Envelope.SetLoopFlag(apuData.square1Data.length_counter_halt);
			break;
		case 0x4001:
			SetSquareData_1_reg(apuData.square1Data, val);
			break;
		case 0x4002:
			SetSquareData_2_reg(apuData.square1Data, val);
			SetDriverFreq(apuData.square1Data.timer, Instrument::square1);
			break;
		case 0x4003:
			SetSquareData_3_reg(apuData.square1Data, val);
			SetDriverFreq(apuData.square1Data.timer, Instrument::square1);
			//std::cout << "writing to 4003 (pulse1 length): " << (int)val << "\n";
			break;
		case 0x4004:
			SetSquareData_0_reg(apuData.square2Data, val, Instrument::square2);
			square2Envelope.SetPeriod(apuData.square2Data.envelope_period);
			square2Envelope.SetLoopFlag(apuData.square2Data.length_counter_halt);
			break;
		case 0x4005:
			SetSquareData_1_reg(apuData.square2Data, val);
			break;
		case 0x4006:
			SetSquareData_2_reg(apuData.square2Data, val);
			SetDriverFreq(apuData.square2Data.timer, Instrument::square2);
			break;
		case 0x4007:
			SetSquareData_3_reg(apuData.square2Data, val);
			SetDriverFreq(apuData.square2Data.timer, Instrument::square2);
			//std::cout << "writing to 4007 (pulse2 length): " << (int)val << "\n";
			break;
		case 0x4008:
			SetTriData_8_reg(apuData.triangleData, val);
			//std::cout << "writing to 4008 (tri lin ctr): " << std::hex << (int)val << "\n";
			break;
		case 0x4009:
			break;
		case 0x400A:
			SetTriData_A_reg(apuData.triangleData, val);
			SetDriverFreq(apuData.triangleData.timer, Instrument::triangle);
			//std::cout << "writing to 400A (freq): " << std::hex << (int)val << "\n";
			break;
		case 0x400B:
			SetTriData_B_reg(apuData.triangleData, val);
			SetDriverFreq(apuData.triangleData.timer, Instrument::triangle);
			//std::cout << "writing to 400B (tri length): " << std::hex << (int)val << "\n";
			break;
		case 0x400C:
			break;
		case 0x400E:
			break;
		case 0x400F:
			break;
		case 0x4015:
			//std::cout << "writing to 4015: " << (int)val << "\n";
			SetStatus(val);
			break;
		case 0x4017:
			apuData.mode = (val & 0x80) ? frameCounterMode::mode_5_step : frameCounterMode::mode_4_step;
			reset_4017_triggered = true;
			float cycles_frac = apuData.APUcycles - (int)(apuData.APUcycles);
			reset_4017_counter = (cycles_frac == 0) ? 3 : 4;
			//std::cout << "writing to 4017 (frame counter): " << std::hex << (int)val << "\n";
			break;
		}
		//whenever there is an update to necessary parameters, do that update.
	}
};