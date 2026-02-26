#include "monocordio.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ADSR Stages
#define ADSR_IDLE    0
#define ADSR_ATTACK  1
#define ADSR_DECAY   2
#define ADSR_SUSTAIN 3
#define ADSR_RELEASE 4

// Internal state
static MC_Channel channels[MC_CHANNELS_COUNT];
static SDL_AudioDeviceID audio_device;
static float master_volume = 0.3f; // Default master volume to prevent clipping

// MML Parser Helper Functions
// Returns frequency in Hz for a given note index (0=C, 1=C#, ..., 11=B) and octave
static float get_note_freq(int note_index, int octave) {
    // Reference: A4 (note index 9, octave 4) = 440Hz
    // MIDI note number = (octave + 1) * 12 + note_index
    // A4 (69) -> (4+1)*12 + 9 = 69
    int midi_note = (octave + 1) * 12 + note_index;
    return 440.0f * powf(2.0f, (midi_note - 69.0f) / 12.0f);
}

// Check if a char is a number
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Parse a number from the string, advancing the cursor
static int parse_number(const char** cursor) {
    int num = 0;
    while (is_digit(**cursor)) {
        num = num * 10 + (**cursor - '0');
        (*cursor)++;
    }
    return num;
}

static void ProcessMML(MC_Channel* channel) {
    if (!channel->mml_active || !channel->mml_cursor) return;

    // While we don't have a wait time, process commands
    while (channel->mml_wait_samples == 0) {
        char cmd = *channel->mml_cursor;
        
        // End of string
        if (cmd == '\0') {
            channel->mml_active = false;
            // Stop sound
            channel->active = false; // Hard stop or Release? Let's release.
            channel->adsr_stage = ADSR_RELEASE; 
            return;
        }

        channel->mml_cursor++; // Advance past command

        // Skip whitespace
        if (cmd == ' ' || cmd == '\t' || cmd == '\n' || cmd == '\r') continue;

        // Convert to uppercase for consistency
        if (cmd >= 'a' && cmd <= 'z') cmd -= 32;
        
        // Handle '#' and '+' as sharp and '-' as flat for note command NOT YET PARSED
        // But A-G are handled below.
        
        switch (cmd) {
            case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
            case 'R': // Rest
            {
                int note_base = 0;
                int note_idx = -1;
                // Map note to index 0-11
                if (cmd == 'C') note_base = 0;
                else if (cmd == 'D') note_base = 2;
                else if (cmd == 'E') note_base = 4;
                else if (cmd == 'F') note_base = 5;
                else if (cmd == 'G') note_base = 7;
                else if (cmd == 'A') note_base = 9;
                else if (cmd == 'B') note_base = 11;
                
                note_idx = note_base;

                // Check for sharp (#) or flat (-)
                // Peek next char safely
                char next_char = *channel->mml_cursor;
                if (next_char == '#' || next_char == '+') {
                    note_idx++;
                    channel->mml_cursor++;
                } else if (next_char == '-') {
                    note_idx--;
                    channel->mml_cursor++;
                }

                // Parse duration if present
                int length = channel->mml_default_len;
                if (is_digit(*channel->mml_cursor)) {
                    length = parse_number(&channel->mml_cursor);
                }
                
                // Calculate duration in samples
                // MML duration is relative to a whole note (1).
                // 4 = quarter note (1 beat), 8 = eighth note (0.5 beat).
                // Samples per beat = (SampleRate * 60) / Tempo
                // Samples per whole note = Samples per beat * 4
                // Duration = (SampleRate * 60 * 4) / (Tempo * Length)
                
                float samples_per_beat = (MC_SAMPLE_RATE * 60.0f) / (float)channel->mml_tempo;
                float samples_total = (samples_per_beat * 4.0f) / (float)length;
                channel->mml_wait_samples = (uint32_t)samples_total;

                // Play note (if not Rest)
                if (cmd != 'R') {
                    float freq = get_note_freq(note_idx, channel->mml_octave);
                    
                    // Trigger Note
                    channel->frequency = freq;
                    channel->phase = 0.0;
                    channel->stage_timer = 0.0f;
                    channel->vfo_timer = 0.0f;
                    channel->active = true;
                    channel->adsr_stage = ADSR_ATTACK; // Restart ADSR
                    channel->current_amplitude = 0.0f; // Start from 0 for attack
                    channel->lfsr_reg = 0xACE1u; // Reset Noise if needed
                } else {
                    // Rest: Start Release phase of previous note immediately
                    // Or keep silent. Usually rests mean silence.
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

            case '>': // Octave Up
                if (channel->mml_octave < 8) channel->mml_octave++;
                break;

            case '<': // Octave Down
                if (channel->mml_octave > 0) channel->mml_octave--;
                break;

            case 'T': // Tempo
                if (is_digit(*channel->mml_cursor)) {
                    int t = parse_number(&channel->mml_cursor);
                    if (t > 0) channel->mml_tempo = t;
                }
                break;

            case 'L': // Default Length
                 if (is_digit(*channel->mml_cursor)) {
                    int l = parse_number(&channel->mml_cursor);
                    if (l > 0) channel->mml_default_len = l;
                }
                break;
        }
    }
}

// Helper to generate noise
static float generate_noise(uint32_t *lfsr) {
    if (*lfsr == 0) *lfsr = 0xACE1u; // Initialize if zero
    uint8_t lsb = *lfsr & 1;
    *lfsr >>= 1;
    if (lsb) *lfsr ^= 0xB400u;
    return (float)(*lfsr) / 65535.0f * 2.0f - 1.0f; // Normalize -1 to 1
}

// Audio callback function
static void AudioCallback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    float *buffer = (float *)stream;
    int samples = len / sizeof(float); // 32-bit float samples

    // Clear buffer
    memset(stream, 0, len);

    // Process all channels
    for (int i = 0; i < samples; i++) {
        float sample_mix = 0.0f;
        float time_step = 1.0f / MC_SAMPLE_RATE;

        for (int ch = 0; ch < MC_CHANNELS_COUNT; ch++) {
            MC_Channel *channel = &channels[ch];
            
            // --- MML Sequencer Logic ---
            if (channel->mml_active) {
                if (channel->mml_wait_samples > 0) {
                    channel->mml_wait_samples--;
                } else {
                    ProcessMML(channel);
                }
            }

            if (!channel->active) continue;

            const MC_Patch *patch = channel->patch;
            float freq = channel->frequency; // Use stored frequency
            
            // --- ADSR Envelope Processing ---
            float envelope = channel->current_amplitude;
            
            switch (channel->adsr_stage) {
                case ADSR_ATTACK:
                    if (patch->adsr.attack > 0.0f) {
                        envelope += (1.0f / patch->adsr.attack) * time_step;
                    } else {
                        envelope = 1.0f;
                    }

                    if (envelope >= 1.0f) {
                        envelope = 1.0f;
                        channel->adsr_stage = ADSR_DECAY;
                    }
                    break;

                case ADSR_DECAY:
                    if (patch->adsr.decay > 0.0f) {
                        envelope -= ((1.0f - patch->adsr.sustain) / patch->adsr.decay) * time_step;
                    } else {
                        envelope = patch->adsr.sustain;
                    }

                    if (envelope <= patch->adsr.sustain) {
                        envelope = patch->adsr.sustain;
                        channel->adsr_stage = ADSR_SUSTAIN;
                    }
                    break;

                case ADSR_SUSTAIN:
                    envelope = patch->adsr.sustain;
                    // Stay in SUSTAIN until MC_Stop() is called (which sets stage to RELEASE)
                    // Or total duration is reached
                    break;

                case ADSR_RELEASE:
                    if (patch->adsr.release > 0.0f) {
                        // Release from current level (which might be sustain or something else if interrupted)
                        // But for simplicity, we usually release from sustain level or current level down to 0
                        // Here we assume linear decay from CURRENT `envelope` down to 0
                        // To clear from current amplitude correctly we might need to store the amplitude at release start
                        // For this implementation, we just decrement by a fixed rate calculated from 1.0 or current?
                        // "The gain goes from current level to 0.0 linearly over adsr.release time"
                        // To do this perfectly we'd need to know the start value of release. 
                        // Simplified approach: decrement based on max amplitude / release time
                        // envelope -= (1.0f / patch->adsr.release) * time_step; // This assumes release from 1.0
                        
                        // Better approach: Since we don't store "release_start_level", we can just decay exponentially or linearly base on 1.0
                        // Castor said: "Release: When MC_Stop is called, gain goes from current level to 0.0"
                        // Let's use a fixed slope based on 1.0 for consistency, or we need an extra state variable.
                        // Let's use the simplest: decay by (1.0 / release_time) * dt.
                         envelope -= (1.0f / patch->adsr.release) * time_step;
                    } else {
                        envelope = 0.0f;
                    }

                    if (envelope <= 0.0f) {
                        envelope = 0.0f;
                        channel->active = false;
                        channel->adsr_stage = ADSR_IDLE;
                    }
                    break;
                    
                default:
                    channel->active = false;
                    break;
            }
            channel->current_amplitude = envelope;
            
            // --- Check Total Duration ---
            // If total_duration is set > 0, we might force a stop
            if (patch->total_duration > 0.0f && channel->adsr_stage != ADSR_RELEASE && channel->adsr_stage != ADSR_IDLE) {
                 channel->stage_timer += time_step;
                 if (channel->stage_timer >= patch->total_duration) {
                     // Force release
                     channel->adsr_stage = ADSR_RELEASE;
                 }
            }


            // Basic Waveform Generation
            float sample_val = 0.0f;
            float t = (float)(channel->phase / (2.0 * M_PI)); // normalized 0..1 cycle

            // VFO logic (basic implementation)
            // Ideally phase increment would change, for simple smoke test keeping constant freq
            
            // --- VFO Logic (Pitch Slide) ---
            if (patch->vfo.duration > 0.0f) {
                float vfo_progress = channel->vfo_timer / patch->vfo.duration;
                if (vfo_progress > 1.0f) vfo_progress = 1.0f;
                
                float offset = patch->vfo.start_offset_hz + vfo_progress * (patch->vfo.end_offset_hz - patch->vfo.start_offset_hz);
                freq += offset;
                
                // Advance VFO timer
                channel->vfo_timer += time_step;
            }

            // Frequency Safety Clamp
            if (freq < 20.0f) freq = 20.0f;
            if (freq > 20000.0f) freq = 20000.0f;
            
            switch (patch->wave) {
                case MC_WAVE_SQUARE:
                    sample_val = (channel->phase < (2.0 * M_PI * patch->duty_cycle)) ? 1.0f : -1.0f;
                    break;
                case MC_WAVE_SINE:
                    sample_val = sinf((float)channel->phase);
                    break;
                case MC_WAVE_SAWTOOTH:
                    sample_val = 2.0f * t - 1.0f;
                    break;
                case MC_WAVE_TRIANGLE:
                     sample_val = 2.0f * fabsf(2.0f * t - 1.0f) - 1.0f;
                    break;
                case MC_WAVE_NOISE_LFSR:
                    sample_val = generate_noise(&channel->lfsr_reg);
                    break;
            }
            
            // Advance Phase
            double phase_increment = (2.0 * M_PI * freq) / MC_SAMPLE_RATE;
            channel->phase += phase_increment;
            if (channel->phase >= 2.0 * M_PI) {
                channel->phase -= 2.0 * M_PI;
            }

            sample_mix += sample_val * envelope; 
        }
        
        // Master Volume & Soft Clipping
        sample_mix *= master_volume;

        if (sample_mix > 1.0f) sample_mix = 1.0f;
        if (sample_mix < -1.0f) sample_mix = -1.0f;

        buffer[i] = sample_mix;
    }
}

bool MC_Init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = MC_SAMPLE_RATE;
    want.format = AUDIO_F32; // Float 32 bit
    want.channels = 1;       // Mono for simplicity in smoke test
    want.samples = 1024;     // Buffer size
    want.callback = AudioCallback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return false;
    }

    // Initialize Channels
    for (int i = 0; i < MC_CHANNELS_COUNT; i++) {
        channels[i].active = false;
        channels[i].adsr_stage = ADSR_IDLE;
        channels[i].current_amplitude = 0.0f;
        channels[i].lfsr_reg = 0xACE1u;
    }

    SDL_PauseAudioDevice(audio_device, 0); // Start playing
    return true;
}

void MC_Cleanup(void) {
    SDL_CloseAudioDevice(audio_device);
    SDL_Quit();
}

void MC_Play(int channel_id, const MC_Patch* patch, float base_freq) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].patch = patch;
    channels[channel_id].frequency = base_freq;
    channels[channel_id].phase = 0.0;
    channels[channel_id].stage_timer = 0.0f;
    channels[channel_id].vfo_timer = 0.0f;
    
    // Start ADSR
    channels[channel_id].active = true;
    channels[channel_id].adsr_stage = ADSR_ATTACK;
    channels[channel_id].current_amplitude = 0.0f;
    
    channels[channel_id].lfsr_reg = 0xACE1u; // Reset noise seed
    SDL_UnlockAudioDevice(audio_device);
}

void MC_Stop(int channel_id) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    if (channels[channel_id].active) {
        channels[channel_id].adsr_stage = ADSR_RELEASE;
        // Don't set active to false yet, wait for release to finish in AudioCallback
    }
    SDL_UnlockAudioDevice(audio_device);
}

void MC_Wait(uint32_t ms) {
    SDL_Delay(ms);
}

void MC_PlayMML(int channel_id, const MC_Patch* patch, const char* mml_string) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].patch = patch;
    
    // Reset MML state
    channels[channel_id].mml_cursor = mml_string;
    channels[channel_id].mml_start = mml_string;
    channels[channel_id].mml_active = true;
    channels[channel_id].mml_wait_samples = 0;
    channels[channel_id].mml_octave = 4; // Default Middle C octave
    channels[channel_id].mml_tempo = 120; // Default BPM
    channels[channel_id].mml_default_len = 4; // Quarter note default
    
    // Reset Sound State
    channels[channel_id].active = false; // Wait for first note
    channels[channel_id].phase = 0.0;
    channels[channel_id].stage_timer = 0.0f;
    channels[channel_id].vfo_timer = 0.0f;
    channels[channel_id].adsr_stage = ADSR_IDLE;
    
    SDL_UnlockAudioDevice(audio_device);
}

