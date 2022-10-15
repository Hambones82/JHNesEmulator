#include <stdint.h>
#include <iostream>
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

	void SetDriverFreq(uint16_t timer, Instrument instrument) {
		audioDriver->DoVoiceCommand(1789773. / (16 * (timer + 1)), VoiceOp::start, instrument);
	}
	void SetSquareData_0_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.duty_cycle = value & 0b1100'0000 >> 6;
		inSquareData.halt_remaining_time = value & 0b0010'0000 >> 5;
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
		SetDriverFreq(apuData.square1Data.timer, Instrument::square1);
	}

	void SetSquareData_3_reg(SquareData& inSquareData, uint8_t value) {
		inSquareData.length_counter = (value & 0b1111'1000) >> 3;
		inSquareData.timer &= 0x00FF;
		inSquareData.timer |= (value & 0b0000'0111) << 8;
		SetDriverFreq(apuData.square1Data.timer, Instrument::square1);
	}
public:
	//for now, just try setting frequency, have the audio driver play samples based on the set freq.
	APU(AudioDriver *in_audioDriver) {
		audioDriver = in_audioDriver;
	}
	void Tick() {
		//update apuData based on timing
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
			break;
		case 0x4001:
			SetSquareData_1_reg(apuData.square1Data, val);
			break;
		case 0x4002:
			SetSquareData_2_reg(apuData.square1Data, val);
			break;
		case 0x4003:
			SetSquareData_3_reg(apuData.square1Data, val);
			break;
		case 0x4004:
			SetSquareData_0_reg(apuData.square2Data, val);
			break;
		case 0x4005:
			SetSquareData_1_reg(apuData.square2Data, val);
			break;
		case 0x4006:
			SetSquareData_2_reg(apuData.square2Data, val);
			break;
		case 0x4007:
			SetSquareData_3_reg(apuData.square2Data, val);
			break;
		}
		//whenever there is an update to necessary parameters, do that update.
	}
};