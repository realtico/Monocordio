#include "mc_internal.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// GLOBAL STATE DEFINITIONS
// ============================================================================

struct MC_Channel channels[MC_CHANNELS_COUNT];
SDL_AudioDeviceID audio_device;
float master_volume = 0.3f; 

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

// Hard/Safeguard Clipping (evitando o tanhf puro que adiciona distorção harmônica a volumes normais)
static float soft_clip(float x) {
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return x;
}

// ============================================================================
// AUDIO CALLBACK (THE HEART)
// ============================================================================

void AudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    
    // ------------------------------------------------------------------------
    // MUNDO 3b (FLUID PLAYER IS HANDLED INTERNALLY BY FLUIDSYNTH)
    // ------------------------------------------------------------------------
    // I/O removed from callback to prevent dropouts and race conditions.

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
                    MC_Internal_ProcessMML(ch);
                }
            }

            if (!ch->active && ch->adsr_stage == ADSR_IDLE) continue;

            // Process the channel (Generates one sample with all effects/envelopes)
            float sample_val = MC_Internal_GenerateSample(ch);
            
            // Mix
            sample_mix += sample_val; 
        }

        // 5. Master Output & Soft Clip
        float final_out = sample_mix * master_volume * MC_INTERNAL_GAIN; 
        buffer[i] = final_out; // Will be clipped after fluid mix
    }

#ifdef MC_MIDI_ENABLED
    // Mix FluidSynth Output (if enabled)
    if (fluid_enabled && fluid_synth) {
       int frames = len / sizeof(float);
       if (frames > 4096) frames = 4096;

       static float fluid_buf[8192]; // frames * 2 channels
       
       // Render stereo from FluidSynth
       if (fluid_synth_write_float(fluid_synth, frames, fluid_buf, 0, 2, fluid_buf, 1, 2) == FLUID_OK) {
           for (int i = 0; i < frames; i++) {
               // Downmix stereo to mono, apply master volume
               float fluid_mono = (fluid_buf[i*2] + fluid_buf[i*2+1]) * 0.5f;
               buffer[i] += fluid_mono * master_volume;
           }
       }
    }
#endif


    // Final Soft Clip Check
    for (int i = 0; i < len / (int)sizeof(float); i++) {
        buffer[i] = soft_clip(buffer[i]);
    }
}

// ============================================================================
// PUBLIC API IMPLEMENTATION (CORE)
// ============================================================================

bool MC_Init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) return false;

#ifdef MC_MIDI_ENABLED
    // FluidSynth Initialization
    if (!MC_Internal_InitFluidSynth()) {
        fprintf(stderr, "[Monocordio] Warning: FluidSynth failed. Continuing in 8-bit only mode.\n");
    }
#endif

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
        channels[i].mode = MC_MODE_INTERNAL; // Default to old school
        channels[i].midi_last_note = -1;
    }

    SDL_PauseAudioDevice(audio_device, 0);
    return true;
}

void MC_Cleanup(void) {
    SDL_PauseAudioDevice(audio_device, 1);
    SDL_CloseAudioDevice(audio_device);
#ifdef MC_MIDI_ENABLED
    MC_Internal_CleanupFluidSynth();
#endif
    SDL_Quit();
}

void MC_Wait(uint32_t ms) {
    SDL_Delay(ms);
}

void MC_SetMasterVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    master_volume = vol;
}

// Exposed control functions that interact with core state
// Functions like MC_Play and MC_Stop could be here or in mc_synth.c
// Given they manipulate channel state primarily used by the mixer, they fit here or synth.
// I will place channel control in mc_core.c as it manages the 'channels' array which is central.
// Actually, MC_Play sets up synthesis parameters. It fits nicely here or separate. 
// I'll keep them here as they are fundamental channel management.

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
    if (ch->lfsr_reg == 0) ch->lfsr_reg = 1; // Prevent lock-up

    SDL_UnlockAudioDevice(audio_device);
}

void MC_Stop(int channel_id) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].adsr_stage = ADSR_RELEASE;
    channels[channel_id].release_start_level = channels[channel_id].current_amplitude;
    channels[channel_id].mml_active = false;
    SDL_UnlockAudioDevice(audio_device);
}

bool MC_IsPlaying(int channel_id) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return false;
    return channels[channel_id].active || (channels[channel_id].adsr_stage != ADSR_IDLE);
}

void MC_SetChannelMode(int channel_id, MC_ChannelMode mode) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].mode = mode;
    SDL_UnlockAudioDevice(audio_device);
}

void MC_SetVolume(int channel_id, float vol) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 2.0f) vol = 2.0f;
    SDL_LockAudioDevice(audio_device);
    channels[channel_id].volume = vol;
    SDL_UnlockAudioDevice(audio_device);
}
