#include "mc_internal.h"
#include <math.h>

// ============================================================================
// CORE SYNTHESIS ENGINE (Mundo 1)
// ============================================================================

float MC_Internal_GenerateSample(struct MC_Channel* ch) {
    if (!ch->active && ch->adsr_stage == ADSR_IDLE) return 0.0f;

    // 1. Calculate Frequency (VFO)
    float current_freq = ch->frequency;
    
    // VFO (Variable Frequency Oscillator - Pitch Sliding)
    if (ch->patch.vfo.duration > 0.0f) {
        float vfo_progress = ch->vfo_timer / ch->patch.vfo.duration;
        if (vfo_progress > 1.0f) vfo_progress = 1.0f;
        
        float offset = ch->patch.vfo.start_offset_hz + 
                       (ch->patch.vfo.end_offset_hz - ch->patch.vfo.start_offset_hz) * vfo_progress;
        current_freq += offset;
        ch->vfo_timer += 1.0f / MC_SAMPLE_RATE;
    }

    // Vibrato (LFO - Low Frequency Oscillator)
    if (ch->patch.vibrato_rate > 0.0f && ch->patch.vibrato_depth > 0.0f) {
        float lfo_val = sin(ch->vibrato_phase);
        current_freq += lfo_val * ch->patch.vibrato_depth;
        ch->vibrato_phase += (ch->patch.vibrato_rate * 2.0 * M_PI) / MC_SAMPLE_RATE;
        if (ch->vibrato_phase >= 2.0 * M_PI) ch->vibrato_phase -= 2.0 * M_PI;
    }

    // 2. Generate Waveform
    float sample_val = 0.0f;
    // Calculate phase increment based on current frequency
    double phase_incr = (current_freq * 2.0 * M_PI) / MC_SAMPLE_RATE;
    
    switch (ch->patch.wave) {
        case MC_WAVE_SINE:
            sample_val = sin(ch->phase);
            ch->phase += phase_incr;
            break;
        
        case MC_WAVE_SQUARE:
            {
                double norm = ch->phase / (2.0 * M_PI);
                norm -= (int)norm; // normalize 0..1
                sample_val = (norm < ch->patch.duty_cycle) ? 1.0f : -1.0f;
            }
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
            // Logic: update LFSR only when phase wraps around
            if (ch->phase >= 2.0 * M_PI) {
                uint8_t lsb = ch->lfsr_reg & 1;
                ch->lfsr_reg >>= 1;
                if (lsb) ch->lfsr_reg ^= 0xB400u; // Tap
                ch->phase -= 2.0 * M_PI; // Wrap phase
            }
            sample_val = (ch->lfsr_reg & 1) ? 0.5f : -0.5f; 
            ch->phase += phase_incr; 
            break;
            
        default:
            // Silence for unknown waves
            sample_val = 0.0f;
            break;
    }

    // Keep Phase in bounds for non-noise (Noise handles wrap differently)
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
                // Release formula fixed: decays from release_start_level
                float start_val = ch->release_start_level;
                if (start_val <= 0.0001f) start_val = 1.0f; // Safety fallback
                
                float rate = start_val / ch->patch.adsr.release;
                ch->current_amplitude -= rate * dt;
                
                if (ch->current_amplitude <= 0.0f) {
                    ch->current_amplitude = 0.0f;
                    ch->adsr_stage = ADSR_IDLE;
                    ch->active = false;
                }
            }
            break;
    }

    // 4. Final Mix Output (Sample * Envelope * Channel Volume * Patch Volume)
    return sample_val * ch->current_amplitude * ch->volume * ch->patch.volume;
}

