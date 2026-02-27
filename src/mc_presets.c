#include <stdio.h>
#include "monocordio.h"
#include "mc_presets.h"

// ============================================================================
// MONOCORDIO GENERAL AUDIO BANK - IMPLEMENTATION
// ============================================================================

// 01: Piano - Sine + Soft Envelope
const MC_Patch MC_PATCH_PIANO = {
    .wave = MC_WAVE_SINE, 
    .adsr = { .attack = 0.01f, .decay = 0.8f, .sustain = 0.0f, .release = 0.05f },
    .vfo = { .duration = 0 },
    .duty_cycle = 0.5f,
    .total_duration = 0 // Infinite sustain usually, but here relies on decay/release
};

// 02: Organ - Hollow Square (25% Duty)
const MC_Patch MC_PATCH_ORGAN = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.05f, .decay = 0.0f, .sustain = 1.0f, .release = 0.1f },
    .duty_cycle = 0.25f
};

// 03: Flute - Sine + Vibrato (VFO not truly vibrato here, reusing Pitch Slide logic for now as VFO)
// Note: The VFO struct only has start/end Hz offset. We can't do LFO vibrato yet.
// We will simulate a slight pitch bend up at start.
const MC_Patch MC_PATCH_FLUTE = {
    .wave = MC_WAVE_TRIANGLE, // Changed to Triangle for breathiness as per some FM synthesis models, or stick to Sine.
    .adsr = { .attack = 0.1f, .decay = 0.0f, .sustain = 0.8f, .release = 0.2f },
    .vfo = { .start_offset_hz = -5.0f, .end_offset_hz = 0.0f, .duration = 0.2f }
};

// 04: Guitar - Saw + Palm Mute
const MC_Patch MC_PATCH_GUITAR = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.0f, .decay = 0.4f, .sustain = 0.0f, .release = 0.1f }
};

// 05: Bass - Triangle + Sustain
const MC_Patch MC_PATCH_BASS = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.02f, .decay = 0.2f, .sustain = 0.8f, .release = 0.1f }
};

// 06: Trumpet - Saw + Attack Pitch Bump
const MC_Patch MC_PATCH_TRUMPET = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.05f, .decay = 0.2f, .sustain = 1.0f, .release = 0.1f },
    .vfo = { .start_offset_hz = -20.0f, .end_offset_hz = 0.0f, .duration = 0.05f }
};

// 07: Strings - Slow Saw
const MC_Patch MC_PATCH_STRINGS = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.6f, .decay = 0.0f, .sustain = 1.0f, .release = 0.8f }
};

// 08: Clarinet - Square (Close to 50% duty)
const MC_Patch MC_PATCH_CLARINET = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.15f, .decay = 0.0f, .sustain = 0.9f, .release = 0.2f },
    .duty_cycle = 0.5f
};

// 09: Pizzicato - Short Pluck
const MC_Patch MC_PATCH_PIZZICATO = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.0f, .decay = 0.2f, .sustain = 0.0f, .release = 0.05f }
};

// --- Percussion ---

// 10: Kick - Sub Bass Drop
const MC_Patch MC_PATCH_KICK = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.0f, .decay = 0.3f, .sustain = 0.0f, .release = 0.0f },
    .vfo = { .start_offset_hz = 60.0f, .end_offset_hz = -100.0f, .duration = 0.1f } // Approximate drop
};

// 11: Snare - Noise Snap
const MC_Patch MC_PATCH_SNARE = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.0f, .decay = 0.25f, .sustain = 0.0f, .release = 0.05f }
};

// 12: Hi-Hat Closed - Tiny tick
const MC_Patch MC_PATCH_HH_CLOSED = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.0f, .decay = 0.05f, .sustain = 0.0f, .release = 0.02f }
};

// 13: Hi-Hat Open - Sizzle
const MC_Patch MC_PATCH_HH_OPEN = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.02f, .decay = 0.5f, .sustain = 0.0f, .release = 0.2f }
};

// 14: Tom - Triangle drop
const MC_Patch MC_PATCH_TOM = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.0f, .decay = 0.4f, .sustain = 0.0f, .release = 0.1f },
    .vfo = { .start_offset_hz = 20.0f, .end_offset_hz = -20.0f, .duration = 0.2f }
};

// 15: Splash - Long Noise
const MC_Patch MC_PATCH_SPLASH = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.05f, .decay = 1.5f, .sustain = 0.0f, .release = 1.0f }
};

// 16: Explosion Elements
const MC_Patch MC_PATCH_EXPLOSION_BODY = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.05f, .decay = 1.5f, .sustain = 0.0f, .release = 0.5f }
};

const MC_Patch MC_PATCH_EXPLOSION_SUB = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.0f, .decay = 0.6f, .sustain = 0.0f, .release = 0.2f },
    .vfo = { .start_offset_hz = 0.0f, .end_offset_hz = -60.0f, .duration = 0.3f }
};

void MC_PlayExplosion(int channel_noise, int channel_sub) {
    // Layer 1: Rumble
    MC_Play(channel_noise, &MC_PATCH_EXPLOSION_BODY, 440.0f); // Pitch doesnt matter much for noise
    
    // Layer 2: Shockwave (Start at 80Hz and slide down due to VFO)
    MC_Play(channel_sub, &MC_PATCH_EXPLOSION_SUB, 80.0f);
}
