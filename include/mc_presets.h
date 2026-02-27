#ifndef MC_PRESETS_H
#define MC_PRESETS_H

#include "monocordio.h"

// ============================================================================
// MONOCORDIO GENERAL MIDI (GM16) - PRESET BANK
// ============================================================================

// --- Melodic Instruments ---
extern const MC_Patch MC_PATCH_PIANO;       // 01: Sine + Soft Envelope
extern const MC_Patch MC_PATCH_ORGAN;       // 02: Square + Hollow Duty
extern const MC_Patch MC_PATCH_FLUTE;       // 03: Sine + Vibrato
extern const MC_Patch MC_PATCH_GUITAR;      // 04: Saw + Palm Mute
extern const MC_Patch MC_PATCH_BASS;        // 05: Triangle + Sustain
extern const MC_Patch MC_PATCH_TRUMPET;     // 06: Saw + Attack Pitch Bump
extern const MC_Patch MC_PATCH_STRINGS;     // 07: Saw + Slow Attack
extern const MC_Patch MC_PATCH_CLARINET;    // 08: Square (50%) + Reed Attack
extern const MC_Patch MC_PATCH_PIZZICATO;   // 09: Triangle + Pluck
extern const MC_Patch MC_PATCH_VIOLIN;      // 10: Sawtooth + Vibrato

// --- Percussion ---
extern const MC_Patch MC_PATCH_KICK;        // 11: Square + Sub Drop
extern const MC_Patch MC_PATCH_SNARE;       // 12: Noise + Snap
extern const MC_Patch MC_PATCH_HH_CLOSED;   // 13: Noise + Tick
extern const MC_Patch MC_PATCH_HH_OPEN;     // 14: Noise + Sizzle
extern const MC_Patch MC_PATCH_TOM;         // 15: Triangle + Pitch Drop
extern const MC_Patch MC_PATCH_SPLASH;      // 16: Noise + Long Decay
extern const MC_Patch MC_PATCH_METAL_ATTACK;// 17: Square High + Short
extern const MC_Patch MC_PATCH_NOISE_TAIL;  // 18: LFSR + Long Decay

// --- SFX ---
extern const MC_Patch MC_PATCH_EXPLOSION_BODY;  // 16a: Noise Rumble
extern const MC_Patch MC_PATCH_EXPLOSION_SUB;   // 16b: Square Sub Drop (Shockwave)

/**
 * Helper to trigger a full "Hollywood" explosion using 2 channels.
 * Uses 'channel_noise' for the rumble and 'channel_sub' for the shockwave.
 */
void MC_PlayExplosion(int channel_noise, int channel_sub);

#endif // MC_PRESETS_H
