#include <stdio.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio: Teste MML Samsung/Mario...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    // Patch 1: Melodia (Square Wave - NES Style)
    MC_Patch melody_patch = {
        .wave = MC_WAVE_SQUARE,
        .duty_cycle = 0.5f,
        .total_duration = 0.0f, // MML controla a duração
        .adsr = { .attack = 0.02f, .decay = 0.1f, .sustain = 0.8f, .release = 0.1f },
        .vfo = { 0 }
    };

    // Patch 2: Baixo (Triangle Wave)
    MC_Patch bass_patch = {
        .wave = MC_WAVE_TRIANGLE,
        .duty_cycle = 0.5f,
        .total_duration = 0.0f,
        .adsr = { .attack = 0.05f, .decay = 0.2f, .sustain = 0.6f, .release = 0.2f },
        .vfo = { 0 }
    };

    // MML Strings
    // Super Mario Bros Theme (Simplified)
    // Intro
    const char* mml_melody = "T180 O4 E8 E8 R8 E8 R8 C8 E8 R8 G4 R4 O3 G4 R4";
    const char* mml_bass   = "T180 O3 D8 D8 R8 D8 R8 < A8 > D8 R8 G4 R4 < G4 R4";

    // Play
    printf("Tocando Tema do Mario (Canal 0: Melody, Canal 1: Bass)...\n");
    MC_PlayMML(0, &melody_patch, mml_melody);
    MC_PlayMML(1, &bass_patch, mml_bass);

    // Wait for the song to finish
    // Duration approx: 12 beats at 180bpm.
    // 60/180 = 0.33s per beat.
    // 12 * 0.33 = 4 seconds approx.
    MC_Wait(4500);

    // Teste Samsung (Melodia Simples)
    printf("Tocando Melodia de Teste (Canal 0)...\n");
    const char* mml_samsung = "T120 O5 C4 D4 E4 F4 G4 A4 B4 > C4";
    MC_PlayMML(0, &melody_patch, mml_samsung);
    MC_Wait(3000);

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
