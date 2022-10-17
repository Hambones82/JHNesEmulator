#include <assert.h>
#include <sdl.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <chrono>
#include <mutex>

export module AudioDriver;
import APUData;

const int AUDIO_BUFFER_SIZE = 2000; //in samples
float TEMPO = 168; //beats per minute
const float SAMPLES_PER_SECOND = 44100.0;
const int SAMPLES_PER_SECOND_INT = 44100;
float SILENCE = 0.0;

//this needs to be rewritten...
//playback for device


void PlayAudio(void* userData, Uint8* stream, int len) {

    //just feed back the last sample
 
    *stream = *(Uint8*)userData;

}
//TODO:
//VoiceRender -- generates samples from voice state
//something w phase?
//current state of voice?

export enum class Instrument { square1, square2, triangle };
export enum class VoiceOp { start, stop };

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
        //std::cout << "voice command begin\n";
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
        //std::cout << "voice command end\n";
    }
    //this might be unused...
    void FillWithSilence(std::array<float, AUDIO_BUFFER_SIZE>& out_buffer, int start_index, int end_index) {
        for (int i = start_index; i < end_index; i++) {
            out_buffer[i] = SILENCE;
        }
    }

    //this is wrong... might also want to insert silence at the beginning of the track...
    float CurrentPhase() {
        //std::cout << "current phase begin\n";
        auto note = track[current_note_track_index].get();

        float raw_phase = current_note_start_phase +
            (current_time_index - note->GetStartTimeIndex()) / SAMPLES_PER_SECOND * note->GetFreq();

        float result = raw_phase - (int)raw_phase;
        //std::cout << "current phase: " << result << "\n";
        if (result < 0 || result > 1)
        {
            std::cout << "phase is out of bounds: " << result << "\n";
        }
        //std::cout << "current phase end\n";
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
            if (current_note_track_index == track.size() - 1) {//the last note
                out_buffer[i] = CurrentNoteSample();
            }
            //if there is a next note and that note is prior to the current time index, set to next note, get sample
            //***THE WHILE LOOP ADVANCES TO THE FIRST NOTE THAT IS AFTER TO THE CURRENT TIME***//
            else if (track[current_note_track_index + 1].get()->GetStartTimeIndex() <= current_time_index) { 
                while (current_note_track_index + 1 < track.size() - 1) {
                    current_note_track_index++;
                    if (!(track[current_note_track_index + 1].get()->GetStartTimeIndex() <= current_time_index)) {
                        break;
                    }
                    else {
                        int j = 0;
                    }
                }
                current_note_start_phase = CurrentPhase();//in waveforms
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

export class AudioDriver {
private:
    std::mutex access_mutex;
    SDL_AudioSpec got_spec;
    SDL_AudioDeviceID device;
    std::chrono::system_clock::time_point start_time;
    Voice triangle{ Instrument::triangle };
    Voice square1{ Instrument::square1 };
    Voice square2{ Instrument::square2 };
    int sampleRate;//expressed as frequency in Hz
    Uint8 last_sample;
    float TimeSinceStart() /*in beats*/ {
        auto now = std::chrono::system_clock::now();
        auto time_since_start = now - start_time;
        auto milliseconds_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_start);
        float current_beat = (milliseconds_since_start.count() / 60000.) * TEMPO;
        return current_beat;
    }
public:
    SDL_AudioDeviceID GetDeviceID() {
        return device;
    }
    AudioDriver() {
        start_time = std::chrono::system_clock::now();//so darn complicated...
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            std::cout << "error initializing audio: ";
            std::cout << SDL_GetError() << "\n";
        }
        SDL_AudioSpec want_spec;
        want_spec.freq = SAMPLES_PER_SECOND;
        want_spec.format = AUDIO_U8;
        want_spec.channels = 1;
        want_spec.samples = AUDIO_BUFFER_SIZE;
        want_spec.callback = NULL;
        //want_spec.userdata = &last_sample;
        
        device = SDL_OpenAudioDevice(NULL, 0, &want_spec, &got_spec, 0);
        SDL_PauseAudioDevice(device, 0);
    }
    void DoVoiceCommand(float freq, float time, VoiceOp op, Instrument instrument) {
        DoVoiceCommand(std::make_unique<VoiceCommand>(freq, time, op, instrument));
    }

    void DoVoiceCommand(float freq, VoiceOp op, Instrument instrument) {
        DoVoiceCommand(freq, TimeSinceStart(), op, instrument);
    }

    void DoVoiceCommand(std::unique_ptr<VoiceCommand> voiceCommand) {
        access_mutex.lock();
        //std::cout << "do voice command start\n";
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
        //std::cout << "do voice command end\n";
        access_mutex.unlock();
        //std::cout << "score: " << ToString();
    }

    float Mix(float triangle, float square1, float square2) {
        return /*triangle * .66 + */square1;// *.16; //+ square2 * .16;
    }
    std::array<float, AUDIO_BUFFER_SIZE> triangle_buffer;
    std::array<float, AUDIO_BUFFER_SIZE> square1_buffer;
    std::array<float, AUDIO_BUFFER_SIZE> square2_buffer;
    //returns a normalized amplitude from 0.0 to 1.0
    void GetSamples(std::array<float, AUDIO_BUFFER_SIZE>& out_buffer) {
        access_mutex.lock();
        triangle.GetNextSamples(triangle_buffer);
        square1.GetNextSamples(square1_buffer);
        square2.GetNextSamples(square2_buffer);
        for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
            out_buffer[i] = Mix(triangle_buffer[i], square1_buffer[i], square2_buffer[i]);
        }
        access_mutex.unlock();
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

std::vector<float> samples;
bool log_samples = false;
export void AudioThread(AudioDriver *audioDriver) {
    auto start_time = std::chrono::system_clock::now();
    //std::chrono::duration<float>  sample_time_increment{ (float)AUDIO_BUFFER_SIZE / (float)SAMPLES_PER_SECOND };
    std::chrono::duration<int, std::ratio<AUDIO_BUFFER_SIZE, SAMPLES_PER_SECOND_INT * 2>> sample_increment{ 1 };
    auto current_time = std::chrono::system_clock::now();
    std::array<float, AUDIO_BUFFER_SIZE> float_sample_buffer = {SILENCE};
    std::array<uint8_t, AUDIO_BUFFER_SIZE> out_sample_buffer;
    //int second_counter = 0;
    bool samples_set = false;
    while (true) {
        current_time = std::chrono::system_clock::now();
        if (current_time - start_time >= 2 * sample_increment) {
            SDL_QueueAudio(audioDriver->GetDeviceID(), out_sample_buffer.data(), AUDIO_BUFFER_SIZE);
            start_time = std::chrono::system_clock::now();
            samples_set = false;
        }
        else if (current_time - start_time >= sample_increment) {
            if (samples_set == false) {
                audioDriver->GetSamples(float_sample_buffer);
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    out_sample_buffer[i] = float_sample_buffer[i] * 255;
                    if (log_samples) samples.push_back(out_sample_buffer[i]);
                }
                samples_set = true;
            }
        }
    }
}
