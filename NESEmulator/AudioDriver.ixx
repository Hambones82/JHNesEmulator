#include <sdl.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>

export module AudioDriver;


const int AUDIO_BUFFER_SIZE = 4000; //in samples
float TEMPO = 180; //beats per minute
float SAMPLES_PER_SECOND = 44100.0;
float SILENCE = 0.0;

int audio_index = 0;

//this needs to be rewritten...
//playback for device

void PlayAudio(void* userData, Uint8* stream, int len) {

    int audio_index = *(int*)userData;
    for (int i = 0; i < len; i++) {

        audio_index++;

        float sin_param_c = (((float)audio_index / 44000.0) * 261.62 * 1.0 * M_PI);

        float sin_param_e = (((float)audio_index / 44000.0) * 329.628 * 2.0 * M_PI);

        float sin_param_g = (((float)audio_index / 44000.0) * 391.995 * 4.0 * M_PI);

        float sin_param_top_c = (((float)audio_index / 44000.0) * 261.62 * 8.0 * M_PI);

        Uint8 result = ((sin(sin_param_c) / 2.0 + .5) * 85) + ((sin(sin_param_e) / 2.0 + .5) * 85)
            /* + ((sin(sin_param_g) / 2.0 + .5) * 65)*/ + ((sin(sin_param_top_c) / 2.0 + .5) * 85);

        stream[i] = result;

    }
    *(int*)userData = audio_index;

}
//TODO:
//VoiceRender -- generates samples from voice state
//something w phase?
//current state of voice?

enum class Instrument { square1, square2, triangle };
enum class VoiceOp { start, stop };

struct VoiceCommand {
private:
    const float note_spacing = 1.0 / 100000.0;
    float freq;
    float time; // in beats -- yes.
    VoiceOp voiceOp;
    Instrument instrument;
    //float phase?
public:
    VoiceCommand(float in_freq, float in_time, VoiceOp inOp, Instrument in_instrument) {
        freq = in_freq;
        time = in_time;
        voiceOp = inOp;
        instrument = in_instrument;
    }

    float JustBefore() {
        return time - note_spacing;
    }

    float StartTime() {
        return time;
    }

    VoiceOp GetOp() { return voiceOp; }

    float GetFreq() { return freq; }

    Instrument GetInstrument() {
        return instrument;
    }

    int GetStartTimeIndex() {
        int retval = (time / TEMPO) * 60.0 * SAMPLES_PER_SECOND;
        return retval; //time is in beats...
    }

    float GetSample(float phase) {
        if (voiceOp == VoiceOp::stop) {
            return SILENCE;
        }
        else switch (instrument) {
        case Instrument::square1:
        case Instrument::square2:
            if (phase >= 0 && phase < .5) {
                return 0;
            }
            else if (phase >= .5 && phase <= 1.0) {
                return 1;
            }
            else {
                std::cout << "sample generation problem in note\n";
                std::cout << "phase: " << phase << "\n";
            }
            break;
        case Instrument::triangle:
        {
            float shifted_phase = phase + .25;
            if (shifted_phase > 1) shifted_phase -= 1.;
            if (shifted_phase >= 0 && shifted_phase < .5) {
                float shifted_phase_2 = shifted_phase * 2;
                return shifted_phase_2;
            }
            else if (shifted_phase >= .5 && shifted_phase <= 1.0) {
                float shifted_phase_2 = (shifted_phase - .5) * 2;
                return 1 - shifted_phase_2;
            }
            else {
                std::cout << "sample generation problem in note\n";
                std::cout << "phase: " << shifted_phase << "\n";
            }
            break;
        }
        default:
            std::cout << "unhandled instrument\n";
            break;
        }

    }

    std::string ToString() {
        std::stringstream sstream;
        sstream << "freq: " + std::to_string(freq) + ", time: " + std::to_string(time);
        sstream << ", command: ";
        switch (voiceOp) {
        case VoiceOp::start:
            sstream << "start\n";
            break;
        case VoiceOp::stop:
            sstream << "stop\n";
            break;
        }
        return sstream.str();
    }
};

class Voice {
private:
    std::vector<std::unique_ptr<VoiceCommand>> track;
    Instrument instrument;

    int current_time_index = 0;
    float current_note_start_phase = 0; //0.0 to 1.0
    int current_note_track_index = 0; //oh yeah, because we have stops...
    
public:
    void DoCommand(std::unique_ptr<VoiceCommand> voiceCommand) {
        std::vector<std::unique_ptr<VoiceCommand>>::iterator it;
        if (track.size() == 0) {
            track.push_back(std::move(voiceCommand));
            return;
        }
        else {
            for (int i = 0; i < track.size(); i++) {
                if (track[i]->StartTime() < voiceCommand->StartTime()) {
                    continue;
                }
                else if (track[i]->StartTime() == voiceCommand->StartTime()) {
                    track[i] = std::move(voiceCommand);
                    return;
                }
                else {
                    track.insert(track.begin() + i, std::move(voiceCommand));
                    return;
                }
            }
            track.push_back(std::move(voiceCommand));
        }
    }
    //this might be unused...
    void FillWithSilence(std::array<float, AUDIO_BUFFER_SIZE>& out_buffer, int start_index, int end_index) {
        for (int i = start_index; i < end_index; i++) {
            out_buffer[i] = SILENCE;
        }
    }

    //this is wrong... might also want to insert silence at the beginning of the track...
    float CurrentPhase() {
        auto note = track[current_note_track_index].get();

        float raw_phase = current_note_start_phase +
            (current_time_index - note->GetStartTimeIndex()) / SAMPLES_PER_SECOND * note->GetFreq();

        float result = raw_phase - (int)raw_phase;
        //std::cout << "current phase: " << result << "\n";
        if (result < 0 || result > 1)
        {
            std::cout << "phase is out of bounds: " << result << "\n";
        }
        return raw_phase - (int)raw_phase;
    }

    //???
    float CurrentNoteSample() {
        //get the sample based on the phase
        float retval = track[current_note_track_index]->GetSample(CurrentPhase());
        //std::cout << "value: " << retval << "\n";
        return retval;
    }

    //probably want a rewind to index thing...

    //i think we should advance to current time, placing all samples into an output buffer
    void GetNextSamples(std::array<float, AUDIO_BUFFER_SIZE>& out_buffer) {

        for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
            current_time_index++;
            //if we have no more note data, continue the note
            if (current_note_track_index == track.size() - 1) {
                out_buffer[i] = CurrentNoteSample();
            }
            //if there is a next note and that note is prior to the current time index, set to next note, get sample
            else if (track[current_note_track_index + 1].get()->GetStartTimeIndex() <= current_time_index) { //this never happens -- not sure why
                current_note_start_phase = CurrentPhase();//in waveforms
                current_note_track_index++;
                out_buffer[i] = CurrentNoteSample();
            }
            //if there is a next note but it's later than now, continue with current note
            else if (track[current_note_track_index + 1].get()->GetStartTimeIndex() > current_time_index) {
                out_buffer[i] = CurrentNoteSample();
            }
            else {
                std::cout << "This should not occur -- this is a sample acquisition case that has not been considered\n";
            }

        }

    }
    Voice(Instrument in_instrument) {
        instrument = in_instrument;
        DoCommand(std::make_unique<VoiceCommand>(1, 0, VoiceOp::stop, instrument));
    }
    std::string ToString() {
        std::stringstream sstream;
        for (auto& s : track) {
            sstream << s->ToString();
        }
        return sstream.str();
    }
};

class Score {
public:
    Voice triangle{ Instrument::triangle };
    Voice square1{ Instrument::square1 };
    Voice square2{ Instrument::square2 };
    int sampleRate;//expressed as frequency in Hz

public:
    void DoVoiceCommand(float freq, float time, VoiceOp op, Instrument instrument) {
        DoVoiceCommand(std::make_unique<VoiceCommand>(freq, time, op, instrument));
    }

    void DoVoiceCommand(std::unique_ptr<VoiceCommand> voiceCommand) {
        switch (voiceCommand->GetInstrument()) {
        case Instrument::triangle:
            triangle.DoCommand(std::move(voiceCommand));
            break;
        case Instrument::square1:
            square1.DoCommand(std::move(voiceCommand));
            break;
        case Instrument::square2:
            square2.DoCommand(std::move(voiceCommand));
            break;
        }
    }

    float Mix(float triangle, float square1, float square2) {
        return triangle * .66 + square1 * .16 + square2 * .16;
    }
    std::array<float, AUDIO_BUFFER_SIZE> triangle_buffer;
    std::array<float, AUDIO_BUFFER_SIZE> square1_buffer;
    std::array<float, AUDIO_BUFFER_SIZE> square2_buffer;
    //returns a normalized amplitude from 0.0 to 1.0
    void GetSamples(std::array<float, AUDIO_BUFFER_SIZE>& out_buffer) {
        triangle.GetNextSamples(triangle_buffer);
        square1.GetNextSamples(square1_buffer);
        square2.GetNextSamples(square2_buffer);
        for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
            out_buffer[i] = Mix(triangle_buffer[i], square1_buffer[i], square2_buffer[i]);
        }
    }

    std::string ToString() {
        std::stringstream sstream;
        sstream << "Triangle:\n========\n" << triangle.ToString()
            << "Square1:\n========\n" << square1.ToString()
            << "Square2:\n========\n" << square2.ToString();
        return sstream.str();
    }
};

//this is probably unused
template<typename T>
struct AudioSample {
    uint64_t audio_index;
    T sample;
};

//circular buffer...
template<int size, typename T>
struct AudioBuffer {
private:
    int write;
    int read;
public:

    std::array<T, size> buffer;

    AudioBuffer() {
        read = 0;
        write = 0;
        buffer = std::array<T, size>{};
    }

    T Read() {
        if ((read == write) || (read == write - 1) || ((write == 0) && (read == size))) {
            return buffer[read];
        }

        auto retval = buffer[read];
        if ((read + 1) % size != write) {
            read++;
            if (read == size) {
                read = 0;
            }
        }

        return retval;
    }

    bool Write(T val) {
        buffer[write] = val;
        int next = (write + 1) % size;
        if (next != read) {
            write = next;
        }
        return true;
    }

    //should have a needs write -- or is empty...  basically... if read != write and if write -1 != read
    std::string GetStatus() {
        std::stringstream outStream;
        for (int i = 0; i < size; i++)
        {
            outStream << buffer[i] << " ";
        }
        outStream << "\nRead ptr: " << read << "\n";
        outStream << "Write ptr: " << write << "\n";
        return outStream.str();
    }
};

template <int size, typename T>
void ReadAndPrintBuffer(AudioBuffer<size, T>& audioBuffer, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << "Read " << audioBuffer.Read() << "\n";
    }
}

template <int size, typename T>
void WriteBuffer(AudioBuffer<size, T>& audioBuffer, std::vector<int> args) {
    for (int& i : args) {
        audioBuffer.Write(i);
    }
}

void AudioThread() {

}
