/*
 * metal_violin.c - Demo of Sprint 07.2 Features
 * Showcases:
 * 1. Violin with Vibrato (LFO)
 * 2. Layered Metallic Percussion (Square + Noise)
 * 3. Deep Sub Explosion
 */

#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"
#include "mc_presets.h"

int main() {
    printf("=== Monocordio Sprint 07.2 Showcase ===\n");

    if (!MC_Init()) {
        fprintf(stderr, "Critical Audio Failure.\n");
        return 1;
    }

    printf("\n[1] The Lonely Violin (Vibrato Test)\n");
    printf("Playing scales with natural vibrato...\n");
    
    // Scale: A4, B4, C#5, D5, E5
    float notes[] = { 440.00f, 493.88f, 554.37f, 587.33f, 659.25f };
    for (int i=0; i<5; i++) {
        MC_Play(0, &MC_PATCH_VIOLIN, notes[i]);
        MC_Wait(800);
        MC_Stop(0);
        MC_Wait(100);
    }

    MC_Wait(500);

    printf("\n[2] Industrial Percussion (Layering Test)\n");
    printf("Synthesizing a complex Crash Cymbal...\n");
    
    // Layer 1: Metal Attack (Square 4kHz, 10% Duty)
    // Layer 2: Noise Tail (LFSR Decay)
    MC_Play(0, &MC_PATCH_METAL_ATTACK, 4000.0f); // High pitch for metallic ring
    MC_Play(1, &MC_PATCH_NOISE_TAIL, 8000.0f);   // Noise freq creates texture
    
    MC_Wait(1500);

    printf("Synthesizing a Heavy Industrial Beat...\n");
    // Pattern: Kick - Hat - Snare - Hat - Kick - Kick - Crash
    for (int i=0; i<2; i++) {
        // Beat 1: Kick
        MC_Play(0, &MC_PATCH_KICK, 60.0f);
        MC_Play(1, &MC_PATCH_EXPLOSION_SUB, 60.0f); // Layer SUB for hugeness
        MC_Wait(250);

        // Beat 2: Hat
        MC_Play(2, &MC_PATCH_HH_CLOSED, 1000.0f);
        MC_Wait(250);

        // Beat 3: Snare
        MC_Play(1, &MC_PATCH_SNARE, 400.0f);
        MC_Wait(250);

        // Beat 4: Hat
        MC_Play(2, &MC_PATCH_HH_CLOSED, 1000.0f);
        MC_Wait(250);
    }
    
    // Final Crash
    printf("CRASH!\n");
    MC_Play(0, &MC_PATCH_METAL_ATTACK, 4000.0f);
    MC_Play(1, &MC_PATCH_NOISE_TAIL, 6000.0f);
    MC_Play(2, &MC_PATCH_KICK, 50.0f);
    
    MC_Wait(2000);

    printf("\n[3] Cinematic Explosion Refined\n");
    MC_PlayExplosion(0, 1);
    MC_Wait(2000);

    MC_Cleanup();
    return 0;
}
