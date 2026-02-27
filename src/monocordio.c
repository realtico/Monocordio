#include "monocordio.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// INTERNAL STRUCTURES (OPAQUE)
// ============================================================================

/* Estado Dinâmico do Canal */
struct MC_Channel {
    bool active;
    double phase;
    float current_amplitude;
    uint8_t adsr_stage; // 0:IDLE, 1:ATTACK, 2:DECAY, 3:SUSTAIN, 4:RELEASE
    float stage_timer;
    float frequency;     // Stores the played frequency
    MC_Patch patch;      // Copy of the patch to avoid pointer issues
    uint32_t lfsr_reg;   // Estado para o ruído
    float vfo_timer;     // Timer independente para VFO
    double vibrato_phase;// Fase do LFO para Vibrato
    float volume;        // Volume do canal

    /* Estado do Sequenciador MML */
    const char* mml_cursor; // Ponteiro atual na string MML
    const char* mml_start;  // Inicio da string
    uint32_t mml_wait_samples; // Contador de espera para a proxima nota
    uint8_t mml_octave;     // Oitava atual (padrao 4)
    uint16_t mml_tempo;     // BPM (padrao 120)
    uint8_t mml_default_len;// Duracao padrao (padrao 4 = seminima)
    bool mml_active;        // Se o sequenciador esta rodando
};

// ADSR Stages
#define ADSR_IDLE    0
#define ADSR_ATTACK  1
#define ADSR_DECAY   2
#define ADSR_SUSTAIN 3
#define ADSR_RELEASE 4

// Global State
static struct MC_Channel channels[MC_CHANNELS_COUNT];
static SDL_AudioDeviceID audio_device;
static float master_volume = 0.3f; 

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static float get_note_freq(int note_index, int octave) {
    // MIDI note calculation
    int midi_note = (octave + 1) * 12 + note_index;
    return 440.0f * powf(2.0f, (midi_note - 69.0f) / 12.0f);
}

// Soft Clipping (Hyperbolic Tangent)
static float soft_clip(float x) {
    return tanhf(x);
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int parse_number(const char** cursor) {
    int num = 0;
    while (is_digit(**cursor)) {
        num = num * 10 + (**cursor - '0');
        (*cursor)++;
    }
    return num;
}

// ============================================================================
// MML PROCESSOR
// ============================================================================

static void ProcessMML(struct MC_Channel* channel) {
    if (!channel->mml_active || !channel->mml_cursor) return;

    // While we don't have a wait time, process commands
    while (channel->mml_active && channel->mml_wait_samples == 0) {
        char cmd = *channel->mml_cursor;
        
        // End of string 
        if (cmd == '\0') {
            channel->mml_active = false;
            channel->adsr_stage = ADSR_RELEASE; 
            return;
        }

        channel->mml_cursor++; 

        // Skip whitespace
        if (cmd == ' ' || cmd == '\t' || cmd == '\n' || cmd == '\r') continue;

        // Convert to uppercase
        if (cmd >= 'a' && cmd <= 'z') cmd -= 32;
        
        switch (cmd) {
            case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
            case 'R': // Rest
            {
                int note_base = 0;
                int note_idx = -1;
                
                if (cmd == 'C') note_base = 0;
                else if (cmd == 'D') note_base = 2;
                else if (cmd == 'E') note_base = 4;
                else if (cmd == 'F') note_base = 5;
                else if (cmd == 'G') note_base = 7;
                else if (cmd == 'A') note_base = 9;
                else if (cmd == 'B') note_base = 11;
                
                note_idx = note_base;

                // Sharps/Flats
                char next = *channel->mml_cursor;
                if (next == '#' || next == '+') {
                    note_idx++;
                    channel->mml_cursor++;
                } else if (next == '-') {
                    note_idx--;
                    channel->mml_cursor++;
                }

                // Duration
                int length = channel->mml_default_len;
                if (is_digit(*channel->mml_cursor)) {
                    length = parse_number(&channel->mml_cursor);
                }
                
                // Calculate samples
                float samples_per_beat = (MC_SAMPLE_RATE * 60.0f) / (float)channel->mml_tempo;
                float samples_total = (samples_per_beat * 4.0f) / (float)length;
                channel->mml_wait_samples = (uint32_t)samples_total;

                // Trigger
                if (cmd != 'R') {
                    float freq = get_note_freq(note_idx, channel->mml_octave);
                    
                    channel->frequency = freq;
                    channel->phase = 0.0;
                    channel->stage_timer = 0.0f;
                    channel->vfo_timer = 0.0f;
                    channel->active = true;
                    channel->adsr_stage = ADSR_ATTACK;
                    channel->current_amplitude = 0.0f; 
                    
                } else {
                     channel->adsr_stage = ADSR_RELEASE;
                }
                break;
            }

            case 'O': // Octave
                if (is_digit(*channel->mml_cursor)) {
                    int oct = parse_number(&channel->mml_cursor);
                    if (oct >= 0 && oct <= 8) channel->mml_octave = oct;
                }
                break;

            case 'T': // Tempo
                 if (is_digit(*channel->mml_cursor)) {
                    channel->mml_tempo = parse_number(&channel->mml_cursor);
                }
                break;

            case 'L': // Length
                if (is_digit(*channel->mml_cursor)) {
                    channel->mml_default_len = parse_number(&channel->mml_cursor);
                }
                break;
                
            case '>': // Octave Up
                if (channel->mml_octave < 8) channel->mml_octave++;
                break;

            case '<': // Octave Down
                if (channel->mml_octave > 0) channel->mml_octave--;
                break;
        }
    }
}

// ============================================================================
// AUDIO CALLBACK (THE HEART)
// ============================================================================

void AudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    float* buffer = (float*)stream;
    int samples = len / sizeof(float);

    memset(stream, 0, len);

    for (int i = 0; i < samples; i++) {
        float sample_mix = 0.0f;

        for (int c = 0; c < MC_CHANNELS_COUNT; c++) {
            struct MC_Channel* ch = &channels[c];
            
            // Handle MML Timing
            if (ch->mml_active) {
                if (ch->mml_wait_samples > 0) {
                    ch->mml_wait_samples--;
                } else {
                    ProcessMML(ch);
                }
            }

            if (!ch->active && ch->adsr_stage == ADSR_IDLE) continue;

            // 1. Calculate Frequency (VFO)
            float current_freq = ch->frequency;
            if (ch->patch.vfo.duration > 0.0f) {
                float vfo_progress = ch->vfo_timer / ch->patch.vfo.duration;
                if (vfo_progress > 1.0f) vfo_progress = 1.0f;
                
                float offset = ch->patch.vfo.start_offset_hz + 
                               (ch->patch.vfo.end_offset_hz - ch->patch.vfo.start_offset_hz) * vfo_progress;
                current_freq += offset;
                ch->vfo_timer += 1.0f / MC_SAMPLE_RATE;
            }

            // 1b. Calculate Vibrato (LFO)
            if (ch->patch.vibrato_rate > 0.0f && ch->patch.vibrato_depth > 0.0f) {
                float lfo_val = sin(ch->vibrato_phase);
                current_freq += lfo_val * ch->patch.vibrato_depth;
                ch->vibrato_phase += (ch->patch.vibrato_rate * 2.0 * M_PI) / MC_SAMPLE_RATE;
                if (ch->vibrato_phase >= 2.0 * M_PI) ch->vibrato_phase -= 2.0 * M_PI;
            }

            // 2. Generate Waveform
            float sample_val = 0.0f;
            double phase_incr = (current_freq * 2.0 * M_PI) / MC_SAMPLE_RATE;
            
            switch (ch->patch.wave) {
                case MC_WAVE_SINE:
                    sample_val = sin(ch->phase);
                    ch->phase += phase_incr;
                    break;
                
                case MC_WAVE_SQUARE:
                    sample_val = (sin(ch->phase) >= 0.0) ? 1.0f : -1.0f;
                    ch->phase += phase_incr;
                    break;
                
                case MC_WAVE_SAWTOOTH:
                    {
                        double norm_phase = ch->phase / (2.0 * M_PI);
                        norm_phase -= (int)norm_phase;
                        sample_val = 2.0f * (float)norm_phase - 1.0f;
                    }
                    ch->phase += phase_incr;
                    break;

                case MC_WAVE_TRIANGLE:
                     {
                        double norm_phase = ch->phase / (2.0 * M_PI);
                        norm_phase -= (int)norm_phase;
                        if (norm_phase < 0.5) 
                            sample_val = 4.0f * (float)norm_phase - 1.0f;
                        else 
                            sample_val = 3.0f - 4.0f * (float)norm_phase;
                     }
                     ch->phase += phase_incr;
                     break;

                case MC_WAVE_NOISE_LFSR:
                    // Improved Noise: Clocked by Frequency
                    if (ch->phase >= 2.0 * M_PI) {
                        uint8_t lsb = ch->lfsr_reg & 1;
                        ch->lfsr_reg >>= 1;
                        if (lsb) ch->lfsr_reg ^= 0xB400u; // Tap
                        ch->phase -= 2.0 * M_PI;
                    }
                    sample_val = (ch->lfsr_reg & 1) ? 0.5f : -0.5f; 
                    ch->phase += phase_incr; 
                    break;
            }

            // Keep Phase in bounds for non-noise
            if (ch->patch.wave != MC_WAVE_NOISE_LFSR) {
                if (ch->phase >= 2.0 * M_PI) ch->phase -= 2.0 * M_PI;
            }

            // 3. ADSR Envelope
            float dt = 1.0f / MC_SAMPLE_RATE;
            ch->stage_timer += dt;

            switch (ch->adsr_stage) {
                case ADSR_ATTACK:
                    if (ch->patch.adsr.attack <= 0.001f) {
                        ch->current_amplitude = 1.0f;
                        ch->adsr_stage = ADSR_DECAY;
                        ch->stage_timer = 0.0f;
                    } else {
                         float step = 1.0f / ch->patch.adsr.attack;
                         ch->current_amplitude += step * dt;
                         if (ch->current_amplitude >= 1.0f) {
                             ch->current_amplitude = 1.0f;
                             ch->adsr_stage = ADSR_DECAY;
                             ch->stage_timer = 0.0f;
                         }
                    }
                    break;

                case ADSR_DECAY:
                     if (ch->patch.adsr.decay <= 0.001f || ch->patch.adsr.sustain >= 1.0f) {
                        ch->current_amplitude = ch->patch.adsr.sustain;
                        ch->adsr_stage = ADSR_SUSTAIN;
                     } else {
                        float range = 1.0f - ch->patch.adsr.sustain;
                        float step = range / ch->patch.adsr.decay;
                        ch->current_amplitude -= step * dt;
                        if (ch->current_amplitude <= ch->patch.adsr.sustain) {
                            ch->current_amplitude = ch->patch.adsr.sustain;
                            ch->adsr_stage = ADSR_SUSTAIN;
                        }
                     }
                    break;

                case ADSR_SUSTAIN:
                    ch->current_amplitude = ch->patch.adsr.sustain;
                    break;

                case ADSR_RELEASE:
                    if (ch->patch.adsr.release <= 0.001f) {
                        ch->current_amplitude = 0.0f;
                        ch->adsr_stage = ADSR_IDLE;
                        ch->active = false;
                    } else {
                        // Release formula fix based on max amplitude assumption for rate
                        float rate = 1.0f / ch->patch.adsr.release;
                        ch->current_amplitude -= rate * dt;
                        
                        if (ch->current_amplitude <= 0.0f) {
                            ch->current_amplitude = 0.0f;
                            ch->adsr_stage = ADSR_IDLE;
                            ch->active = false;
                        }
                    }
                    break;
            }

            // 4. Mix
            sample_mix += sample_val * ch->current_amplitude * ch->volume * ch->patch.volume;
        }

        // 5. Master Output & Soft Clip
        float final_out = sample_mix * master_volume * 1.5f; 
        buffer[i] = soft_clip(final_out);
    }
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

bool MC_Init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) return false;

    srand(time(NULL)); 

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = MC_SAMPLE_RATE;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 1024;
    want.callback = AudioCallback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) return false;

    for(int i=0; i<MC_CHANNELS_COUNT; i++) {
        memset(&channels[i], 0, sizeof(struct MC_Channel));
        channels[i].lfsr_reg = 0xACE1u;
        channels[i].volume = 1.0f;
        channels[i].mml_tempo = 120;
        channels[i].mml_default_len = 4;
        channels[i].mml_octave = 4;
    }

    SDL_PauseAudioDevice(audio_device, 0);
    return true;
}

void MC_Cleanup(void) {
    SDL_CloseAudioDevice(audio_device);
    SDL_Quit();
}

void MC_Play(int channel_id, const MC_Patch* patch, float base_freq) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    struct MC_Channel* ch = &channels[channel_id];
    
    // Stop MML to avoid race condition
    ch->mml_active = false;
    
    ch->active = true;
    ch->patch = *patch;
    ch->frequency = base_freq;
    ch->phase = 0.0;
    ch->adsr_stage = ADSR_ATTACK;
    ch->stage_timer = 0.0f;
    ch->current_amplitude = 0.0f;
    ch->vfo_timer = 0.0f;
    ch->lfsr_reg = (rand() % 0xFFFF) ^ 0xACE1u;

    SDL_UnlockAudioDevice(audio_device);
}

void MC_PlayMML(int channel_id, const MC_Patch* patch, const char* mml_string) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;

    SDL_LockAudioDevice(audio_device);
    struct MC_Channel* ch = &channels[channel_id];
    
    ch->active = true;
    ch->patch = *patch;
    ch->mml_start = mml_string;
    ch->mml_cursor = mml_string;
    ch->mml_wait_samples = 0;
    ch->mml_octave = 4;
    ch->mml_tempo = 120;
    ch->mml_default_len = 4;
    ch->mml_active = true;
    
    SDL_UnlockAudioDevice(audio_device);
}

void MC_Stop(int channel_id) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].adsr_stage = ADSR_RELEASE;
    channels[channel_id].mml_active = false;
    SDL_UnlockAudioDevice(audio_device);
}

void MC_Wait(uint32_t ms) {
    SDL_Delay(ms);
}

bool MC_IsPlaying(int channel_id) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return false;
    return channels[channel_id].active || (channels[channel_id].adsr_stage != ADSR_IDLE);
}

void MC_SetVolume(int channel_id, float vol) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 2.0f) vol = 2.0f;
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].volume = vol;
    SDL_UnlockAudioDevice(audio_device);
}

void MC_SetMasterVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    master_volume = vol;
}

