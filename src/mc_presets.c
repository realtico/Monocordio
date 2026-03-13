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
    .volume = 0.6f
};

// 02: Organ - Hollow Square (25% Duty)
const MC_Patch MC_PATCH_ORGAN = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.05f, .decay = 0.0f, .sustain = 1.0f, .release = 0.1f },
    .duty_cycle = 0.25f,
    .volume = 0.5f
};

// 03: Flute - Sine + Vibrato
const MC_Patch MC_PATCH_FLUTE = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.1f, .decay = 0.0f, .sustain = 0.8f, .release = 0.2f },
    .vfo = { .start_offset_hz = -5.0f, .end_offset_hz = 0.0f, .duration = 0.2f },
    .volume = 0.6f
};

// 04: Guitar - Saw + Palm Mute
const MC_Patch MC_PATCH_GUITAR = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.0f, .decay = 0.4f, .sustain = 0.0f, .release = 0.1f },
    .volume = 0.5f
};

// 05: Bass - Triangle + Sustain
const MC_Patch MC_PATCH_BASS = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.02f, .decay = 0.2f, .sustain = 0.8f, .release = 0.1f },
    .volume = 0.7f
};

// 06: Trumpet - Saw + Attack Pitch Bump
const MC_Patch MC_PATCH_TRUMPET = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.05f, .decay = 0.2f, .sustain = 1.0f, .release = 0.1f },
    .vfo = { .start_offset_hz = -20.0f, .end_offset_hz = 0.0f, .duration = 0.05f },
    .volume = 0.6f
};

// 07: Strings - Slow Saw
const MC_Patch MC_PATCH_STRINGS = {
    .wave = MC_WAVE_SAWTOOTH, 
    .adsr = { .attack = 0.6f, .decay = 0.0f, .sustain = 1.0f, .release = 0.8f },
    .volume = 0.4f
};

// 08: Clarinet - Square (Close to 50% duty)
const MC_Patch MC_PATCH_CLARINET = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.15f, .decay = 0.0f, .sustain = 0.9f, .release = 0.2f },
    .duty_cycle = 0.5f,
    .volume = 0.5f
};

// 09: Pizzicato - Short Pluck
const MC_Patch MC_PATCH_PIZZICATO = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.0f, .decay = 0.2f, .sustain = 0.0f, .release = 0.05f },
    .volume = 0.7f
};

// 10: Violin - Sawtooth + Vibrato
const MC_Patch MC_PATCH_VIOLIN = {
    .wave = MC_WAVE_SAWTOOTH,
    .adsr = { .attack = 0.15f, .decay = 0.2f, .sustain = 0.8f, .release = 0.3f },
    .vibrato_rate = 5.0f,  // 5Hz
    .vibrato_depth = 5.0f, // +/- 5Hz
    .volume = 0.6f
};


// --- Percussion ---

// 11: Kick - Sub Bass Drop
const MC_Patch MC_PATCH_KICK = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.0f, .decay = 0.3f, .sustain = 0.0f, .release = 0.0f },
    .vfo = { .start_offset_hz = 60.0f, .end_offset_hz = -100.0f, .duration = 0.1f },
    .volume = 0.9f
};

// 12: Snare - Noise Snap
const MC_Patch MC_PATCH_SNARE = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.0f, .decay = 0.25f, .sustain = 0.0f, .release = 0.05f },
    .volume = 0.7f
};

// 13: Hi-Hat Closed - Tiny tick
const MC_Patch MC_PATCH_HH_CLOSED = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.0f, .decay = 0.05f, .sustain = 0.0f, .release = 0.02f },
    .volume = 0.4f
};

// 14: Hi-Hat Open - Sizzle
const MC_Patch MC_PATCH_HH_OPEN = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.02f, .decay = 0.5f, .sustain = 0.0f, .release = 0.2f },
    .volume = 0.4f
};

// 15: Tom - Triangle drop
const MC_Patch MC_PATCH_TOM = {
    .wave = MC_WAVE_TRIANGLE, 
    .adsr = { .attack = 0.0f, .decay = 0.4f, .sustain = 0.0f, .release = 0.1f },
    .vfo = { .start_offset_hz = 20.0f, .end_offset_hz = -20.0f, .duration = 0.2f },
    .volume = 0.7f
};

// 16: Splash - Long Noise
const MC_Patch MC_PATCH_SPLASH = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.05f, .decay = 1.5f, .sustain = 0.0f, .release = 1.0f },
    .volume = 0.5f
};

// 17-18: Cymbal Layers
const MC_Patch MC_PATCH_METAL_ATTACK = {
    .wave = MC_WAVE_SQUARE,
    .duty_cycle = 0.1f, // 10%
    .adsr = { .attack = 0.0f, .decay = 0.05f, .sustain = 0.0f, .release = 0.0f },
    .volume = 0.6f
};

const MC_Patch MC_PATCH_NOISE_TAIL = {
    .wave = MC_WAVE_NOISE_LFSR,
    .adsr = { .attack = 0.0f, .decay = 1.2f, .sustain = 0.0f, .release = 0.2f },
    .volume = 0.5f
};


// 16: Explosion Elements
const MC_Patch MC_PATCH_EXPLOSION_BODY = {
    .wave = MC_WAVE_NOISE_LFSR, 
    .adsr = { .attack = 0.05f, .decay = 1.5f, .sustain = 0.0f, .release = 0.5f },
    .volume = 0.8f
};

const MC_Patch MC_PATCH_EXPLOSION_SUB = {
    .wave = MC_WAVE_SQUARE, 
    .adsr = { .attack = 0.0f, .decay = 0.6f, .sustain = 0.0f, .release = 0.2f },
    .vfo = { .start_offset_hz = 0.0f, .end_offset_hz = -40.0f, .duration = 0.2f },
    .volume = 0.9f
};

void MC_PlayExplosion(int channel_noise, int channel_sub) {
    MC_Play(channel_noise, &MC_PATCH_EXPLOSION_BODY, 440.0f);
    MC_Play(channel_sub, &MC_PATCH_EXPLOSION_SUB, 80.0f);
}
