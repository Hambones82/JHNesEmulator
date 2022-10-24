#include <assert.h>
#include <sdl.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <chrono>
#include <mutex>
#include "AudioFile.h"

export module AudioDriver;
import APUData;
import NESConstants;

const int AUDIO_BUFFER_SIZE = 300; //in samples
const int AUDIO_SAMPLE_CHUNK = 1;
float TEMPO = 168; //beats per minute
constexpr float SAMPLES_PER_SECOND = 44100.0;
const int SAMPLES_PER_SECOND_INT = 44100;
float SILENCE = 0.5;
int AudioBitDepth = 256;

class AudioBufferData {
public:
    std::array<std::array<int, AUDIO_BUFFER_SIZE>, 2> callbackBuffers;
    int nextBufferToWrite = 0; //0 or 1
    int nextBufferToRead = 0; //0 or 1
    std::array<bool, 2> bufferReadyToRead = { false, false };
    std::array<bool, 2> bufferReadyToWrite = { true, true };
    std::array<std::mutex, 2> audioBufferMutexes;
    int lastSample = SILENCE * 256;
};

void PlayAudio(void* userData, Uint8* stream, int len) {
    AudioBufferData* aBufferData = (AudioBufferData*)userData;
    int readBuffer = aBufferData->nextBufferToRead;

    if (!aBufferData->bufferReadyToRead[readBuffer]) {
        for (int i = 0; i < len; i++) {
            stream[i] = aBufferData->lastSample;
        }
    }
    else {
        aBufferData->audioBufferMutexes[readBuffer].lock();
        int i = 0;
        for (i; i < len; i++) {
            stream[i] = aBufferData->callbackBuffers[readBuffer][i];
        }
        aBufferData->lastSample = stream[i];
        aBufferData->bufferReadyToWrite[readBuffer] = true;
        aBufferData->bufferReadyToRead[readBuffer] = false;
        aBufferData->nextBufferToRead = 1 - aBufferData->nextBufferToRead; 
        aBufferData->audioBufferMutexes[readBuffer].unlock();
    }
}

export enum class Instrument { square1, square2, triangle, noise };
export enum class VoiceOp { start, stop, volume };
enum class VoiceState {stopped, terminating, playing, };

class Voice {
private:
    Instrument instrument;
    float freq;
    float start_time_index;
    VoiceOp voiceOp;

    float gain = 1.0;

    int current_time_index = 0;
    float saved_time_index = 0;
    float saved_freq = 0;
    float saved_phase = 0;
    bool phase_end = false;

    //need shift mode
    uint16_t noise_sr = 1;
    float noise_tracker = 0;
    uint8_t noise_mode = 0;
    
    VoiceState voiceState = VoiceState::stopped;
    
    void AdvanceNoise(int times) {
        for (int i = 0; i < times; i++) {
            uint16_t mode_mask = noise_mode ? 0b0000'0000'0100'0000 : 2;
            uint8_t noise_shift = noise_mode ? 5 : 1;
            uint8_t xorbit = (noise_sr & 0x0001) ^ ((noise_sr & mode_mask) >> noise_shift);
            noise_sr >>= 1;
            noise_sr &= 0b0011'1111'1111'1111;
            noise_sr |= xorbit << 14;
        }
    }

    float GetSample(float phase) {
        switch (instrument) {
        case Instrument::square1:
        case Instrument::square2:
            if (voiceOp == VoiceOp::stop) {
                return SILENCE;
            }
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
        case Instrument::noise:
        {
            if (voiceOp == VoiceOp::stop) {
                return SILENCE;
            }
            else {
                constexpr float cycles_per_sample = NESCPUFreq / SAMPLES_PER_SECOND;
                float advances_per_sample = cycles_per_sample / freq;
                noise_tracker += advances_per_sample;
                int adv_int = (int)noise_tracker;
                AdvanceNoise(adv_int);
                noise_tracker -= adv_int;
                return noise_sr & 1;
                break;
            }
        }
        default:
            std::cout << "unhandled instrument\n";
            break;
        }

    }

    //to ensure sample continuity, when a first note flows directly into a second note,
    //this function adjusts the saved time index to be as if the second note were playing the whole time
    void AdjustTimeIndex() {
        float retPhase = (current_time_index - saved_time_index) / SAMPLES_PER_SECOND * saved_freq;
        float frac = retPhase - (int)retPhase;
        int time_index_dec = frac * SAMPLES_PER_SECOND / freq;
        saved_time_index = current_time_index - time_index_dec;
    }

public:
    void SetNoiseMode(uint8_t in_nm) {
        noise_mode = in_nm;
    }
    void DoCommand(float in_freq, float in_gain, VoiceOp inOp, Instrument in_instrument) {
        if (inOp == VoiceOp::volume) {
            gain = in_gain;
        }
        else {
            freq = in_freq;
            voiceOp = inOp;
            start_time_index = current_time_index;
        }
    }

    float CurrentPhase(int current_time_index) {
        float retPhase;
        //determine new state based on current note and previous state
        switch (voiceState) {
        case VoiceState::stopped:
            if (voiceOp == VoiceOp::start) {
                saved_time_index = start_time_index;
                voiceState = VoiceState::playing;
            }
            break;
        case VoiceState::terminating: 
            if (voiceOp == VoiceOp::start) {
                voiceState = VoiceState::playing;
                AdjustTimeIndex();
            }
            else if (phase_end) {
                voiceState = VoiceState::stopped;
                phase_end = false;
            }
            break;
        case VoiceState::playing:
            if (voiceOp == VoiceOp::stop) {
                voiceState = VoiceState::terminating;
            }
            else if ((voiceOp == VoiceOp::start) && (saved_freq != freq)) {
                AdjustTimeIndex();
            }
            break;
        }
        //set phase based on new/current state.
        switch (voiceState) {
        case VoiceState::stopped:
            retPhase = SILENCE;
            break;
        case VoiceState::playing:
            saved_freq = freq;
            retPhase = (current_time_index - saved_time_index) / SAMPLES_PER_SECOND * saved_freq;
            break;
        case VoiceState::terminating:
            retPhase = (current_time_index - saved_time_index) / SAMPLES_PER_SECOND * saved_freq;
            break;
        }
       
        if (((int)retPhase != (int)saved_phase) && (voiceState == VoiceState::terminating)){
            phase_end = true;
        }

        saved_phase = retPhase;

        float result = retPhase - (int)retPhase;
        
        if (result < 0 || result > 1)
        {
            std::cout << "phase is out of bounds: " << result << "\n";
        }

        return result;
    }
    
    float CurrentNoteSample() {
        return GetSample(CurrentPhase(current_time_index)) * gain;
    }

    void GetNextSamples(std::array<float, AUDIO_SAMPLE_CHUNK>& out_buffer) {
        for (int i = 0; i < AUDIO_SAMPLE_CHUNK; i++) {
            current_time_index++;
            out_buffer[i] = CurrentNoteSample();
        }
    }
    Voice(Instrument in_instrument) {
        instrument = in_instrument;
        DoCommand(1, 0, VoiceOp::stop, instrument);
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
    Voice noise{ Instrument::noise };
    int sampleRate;//expressed as frequency in Hz
    Uint8 last_sample;
    float TimeSinceStart() /*in beats*/ {
        auto now = std::chrono::system_clock::now();
        auto time_since_start = now - start_time;
        auto milliseconds_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_start);
        float current_beat = (milliseconds_since_start.count() / 60000.) * TEMPO;
        return current_beat;
    }
    bool en_triangle = true;
    bool en_square1 = true;
    bool en_square2 = true;
    bool en_noise = true;
    
public:
    AudioBufferData aBufferData;
    void SetTriangleEnabled(uint8_t value) {
        en_triangle = value ? true : false;
    }
    void SetSquare2Enabled(uint8_t value) {
        en_square2 = value ? true : false;
    }
    void SetSquare1Enabled(uint8_t value) {
        en_square1 = value ? true : false;
    }
    void SetNoiseEnabled(uint8_t value) {
        en_noise = value ? true : false;
    }
    SDL_AudioDeviceID GetDeviceID() {
        return device;
    }
    void Reset() {
        start_time = std::chrono::system_clock::now();//so darn complicated...
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
        want_spec.callback = PlayAudio;
        want_spec.userdata = &aBufferData;
        
        device = SDL_OpenAudioDevice(NULL, 0, &want_spec, &got_spec, 0);
        SDL_PauseAudioDevice(device, 0);
    }
    
    void SetNoiseMode(uint8_t nm) {
        noise.SetNoiseMode(nm);
    }

    void DoVoiceCommand(float freq, VoiceOp op, Instrument instrument) {
        DoVoiceCommand(freq, 1.0, op, instrument);
    }

    void DoVolumeCommand(float gain, Instrument instrument) {
        DoVoiceCommand(1.0, gain, VoiceOp::volume, instrument);
    }

    void DoVoiceCommand(float freq, float gain, VoiceOp op, Instrument instrument) {
        access_mutex.lock();
        switch (instrument) {
        case Instrument::triangle:
            triangle.DoCommand(freq, gain, op, instrument);
            break;
        case Instrument::square1:
            square1.DoCommand(freq, gain, op, instrument);
            break;
        case Instrument::square2:
            square2.DoCommand(freq, gain, op, instrument);
            break;
        case Instrument::noise:
            noise.DoCommand(freq, gain, op, instrument);
        }
        access_mutex.unlock();
    }

    float Mix(float triangle, float square1, float square2, float noise) {
        
        return triangle * en_triangle * .55 + square1 * en_square1 * .15 + square2 * en_square2 * .15 + noise * .15;
    }
    std::array<float, AUDIO_SAMPLE_CHUNK> triangle_buffer;
    std::array<float, AUDIO_SAMPLE_CHUNK> square1_buffer;
    std::array<float, AUDIO_SAMPLE_CHUNK> square2_buffer;
    std::array<float, AUDIO_SAMPLE_CHUNK> noise_buffer;
    //returns a normalized amplitude from 0.0 to 1.0
    void GetSamples(std::array<float, AUDIO_SAMPLE_CHUNK>& out_buffer) {
        access_mutex.lock();
        triangle.GetNextSamples(triangle_buffer);
        square1.GetNextSamples(square1_buffer);
        square2.GetNextSamples(square2_buffer);
        noise.GetNextSamples(noise_buffer);
        for (int i = 0; i < AUDIO_SAMPLE_CHUNK; i++) {
            //std::cout << "square 1 samples: " << (int)square1_buffer[i] << "\n";
            out_buffer[i] = Mix(triangle_buffer[i], square1_buffer[i], square2_buffer[i], noise_buffer[i]);
        }
        access_mutex.unlock();
    }
};

std::vector<float> samples;
bool log_samples = false;
bool log_samples_triggered = false;
int sample_counter = 0;
int samples_logged = 0;
export void AudioThread(AudioDriver *audioDriver) {
    std::array<float, AUDIO_SAMPLE_CHUNK> float_sample_buffer = { SILENCE };
    std::array<uint8_t, AUDIO_BUFFER_SIZE> out_sample_buffer;
    int amount_in_buffer = 0;

    //log samples
    float seconds = 15;
    float num_samples = seconds * SAMPLES_PER_SECOND;

    AudioFile<float> audioFile;
    audioFile.setNumChannels(1);
    audioFile.setBitDepth(8);
    audioFile.setSampleRate(SAMPLES_PER_SECOND);
    audioFile.setNumSamplesPerChannel(num_samples);

    //end log samples

    while (true) {
        sample_counter++;
        /*
        if ((sample_counter > 1000000) && !log_samples_triggered) {
            log_samples = true;
            log_samples_triggered = true;
        }*/
        int writeBuff = audioDriver->aBufferData.nextBufferToWrite;
        if (audioDriver->aBufferData.bufferReadyToWrite[writeBuff]) {
            audioDriver->GetSamples(float_sample_buffer);
            for (int i = 0; i < AUDIO_SAMPLE_CHUNK; i++) {
                out_sample_buffer[i+amount_in_buffer] = float_sample_buffer[i] * 255;
                if (log_samples) {
                    //samples.push_back(out_sample_buffer[i + amount_in_buffer]);
                    audioFile.samples[0][samples_logged++] = float_sample_buffer[i];
                    if (samples_logged == num_samples) {
                        std::string filepath = R"(C:\Users\Josh\source\SDLAudioTest\outputWavs\output1.wav)";
                        audioFile.save(filepath, AudioFileFormat::Wave);
                        log_samples = false;
                    }
                }
            }
            //if we've written the entire buffer, then we can lock it and write...
            amount_in_buffer += AUDIO_SAMPLE_CHUNK;
            if (amount_in_buffer >= AUDIO_BUFFER_SIZE) {
                audioDriver->aBufferData.audioBufferMutexes[writeBuff].lock();
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    audioDriver->aBufferData.callbackBuffers[writeBuff][i] = out_sample_buffer[i];
                }
                audioDriver->aBufferData.bufferReadyToRead[writeBuff] = true;
                audioDriver->aBufferData.bufferReadyToWrite[writeBuff] = false;
                audioDriver->aBufferData.nextBufferToWrite = 1 - audioDriver->aBufferData.nextBufferToWrite;
                audioDriver->aBufferData.audioBufferMutexes[writeBuff].unlock();
                amount_in_buffer -= AUDIO_BUFFER_SIZE;
            }
        }
    }
}
