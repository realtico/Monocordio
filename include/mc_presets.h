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

// --- Percussion ---
extern const MC_Patch MC_PATCH_KICK;        // 10: Square + Sub Drop
extern const MC_Patch MC_PATCH_SNARE;       // 11: Noise + Snap
extern const MC_Patch MC_PATCH_HH_CLOSED;   // 12: Noise + Tick
extern const MC_Patch MC_PATCH_HH_OPEN;     // 13: Noise + Sizzle
extern const MC_Patch MC_PATCH_TOM;         // 14: Triangle + Pitch Drop
extern const MC_Patch MC_PATCH_SPLASH;      // 15: Noise + Long Decay

// --- SFX ---
extern const MC_Patch MC_PATCH_EXPLOSION_BODY;  // 16a: Noise Rumble
extern const MC_Patch MC_PATCH_EXPLOSION_SUB;   // 16b: Square Sub Drop (Shockwave)

/**
 * Helper to trigger a full "Hollywood" explosion using 2 channels.
 * Uses 'channel_noise' for the rumble and 'channel_sub' for the shockwave.
 */
void MC_PlayExplosion(int channel_noise, int channel_sub);

#endif // MC_PRESETS_H
