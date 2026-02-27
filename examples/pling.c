#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio: Teste Pling (ADSR)...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    // Configuração do Patch "Pling"
    // Attack: 0.01s (Curto)
    // Decay: 0.1s
    // Sustain: 0.3
    // Release: 0.5s (Cauda longa)
    MC_Patch pling_patch = {
        .wave = MC_WAVE_SINE, // Sine wave for a cleaner "bling" sound
        .duty_cycle = 0.5f,
        .total_duration = 2.0f, // Max duration just in case
        .adsr = { .attack = 0.01f, .decay = 0.1f, .sustain = 0.3f, .release = 0.5f },
        .vfo = { 0 },
        .volume = 0.6f
    };

    printf("Disparando 3 Pings...\n");

    // Ping 1
    printf("Pling 1 (440Hz)...\n");
    MC_Play(0, &pling_patch, 440.0f);
    MC_Wait(500); // Wait in sustain
    MC_Stop(0);     // Trigger release
    MC_Wait(600); // Wait for release to finish + silence

    // Ping 2
    printf("Pling 2 (554Hz)...\n");
    MC_Play(1, &pling_patch, 554.37f); // C#5
    MC_Wait(500);
    MC_Stop(1);
    MC_Wait(600);

    // Ping 3
    printf("Pling 3 (659Hz)...\n");
    MC_Play(2, &pling_patch, 659.25f); // E5
    MC_Wait(500);
    MC_Stop(2);
    MC_Wait(600);

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
