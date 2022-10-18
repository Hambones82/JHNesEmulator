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



export class APU {
private:
	APUData apuData;
	AudioDriver* audioDriver;
	std::array<uint8_t, 0x20> length_lut{
			10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
	};
	uint8_t LengthValueLookup(uint8_t counter_value) {
		return length_lut[counter_value];
	}

	void SetDriverFreq(uint16_t timer, Instrument instrument) {
		float new_freq = 1789773. / (16 * (timer + 1));
		if ((new_freq >= 20) && (new_freq < 14000)) {
			audioDriver->DoVoiceCommand(new_freq, VoiceOp::start, instrument);
		}
		else {
			audioDriver->DoVoiceCommand(0, VoiceOp::stop, instrument);
		}
	}
	void StopDriver(Instrument instrument) {
		audioDriver->DoVoiceCommand(1, VoiceOp::stop, instrument);
	}
	void SetSquareData_0_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.duty_cycle = value & 0b1100'0000 >> 6;
		inSquareData.length_counter_halt = value & 0b0010'0000 >> 5;
		inSquareData.envelope_flag = (value & 0b0001'0000 >> 4) ? EnvelopeFlag::constant_time : EnvelopeFlag::envelope;
		inSquareData.envelope_period = value & 0x0F;
	}

	void SetSquareData_1_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.sweep_enabled = value & 0b1000'0000 >> 7;
		inSquareData.sweep_period = value & 0b0111'0000 >> 4;
		inSquareData.negate = value & 0b0000'1000 >> 3;
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
		inTriData.linear_counter = value & 0x7F;
	}
	void SetTriData_A_reg(TriangleData& inTriData, uint8_t value) {
		inTriData.timer &= 0xFF00;
		inTriData.timer |= value;
	}
	void SetTriData_B_reg(TriangleData& inTriData, uint8_t value) {
		inTriData.timer &= 0x00FF;
		inTriData.timer |= ((uint16_t)value & 0x07) << 8;
		inTriData.length_counter = LengthValueLookup((value & 0xF8) >> 3);
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
				if (apuData.triangleData.length_counter == 0) {
					StopDriver(Instrument::triangle);
				}
			}
		}
	}
	void ClockEnvTriLin() {

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
			SetSquareData_0_reg(apuData.square1Data, val);
			//std::cout <<std::hex<< "writing to 4000 (duty cycle, lenght halt, volume): " << (int)val << "\n";
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
			SetSquareData_0_reg(apuData.square2Data, val);
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
			break;
		case 0x4009:
			break;
		case 0x400A:
			SetTriData_A_reg(apuData.triangleData, val);
			SetDriverFreq(apuData.triangleData.timer, Instrument::triangle);
			break;
		case 0x400B:
			SetTriData_B_reg(apuData.triangleData, val);
			SetDriverFreq(apuData.triangleData.timer, Instrument::triangle);
			break;
		case 0x4015:
			//std::cout << "writing to 4015: " << (int)val << "\n";
			break;
		case 0x4017:
			apuData.mode = (val & 0x80) ? frameCounterMode::mode_5_step : frameCounterMode::mode_4_step;
			break;
		}
		//whenever there is an update to necessary parameters, do that update.
	}
};