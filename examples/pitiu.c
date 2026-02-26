#include <stdio.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio: Teste Laser (VFO)...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    // Configuração do Patch "Laser Pitiu"
    // Patch: MC_WAVE_SQUARE.
    // Base: 880Hz.
    // VFO: Início +0Hz, Fim -600Hz, Duração 0.1s.
    // ADSR: Attack instantâneo, Decay rápido (0.1s), Sustain 0.0.
    MC_Patch laser_patch = {
        .wave = MC_WAVE_SQUARE,
        .duty_cycle = 0.5f,
        .total_duration = 0.2f, // Slightly longer than decay to ensure silence
        .adsr = { .attack = 0.0f, .decay = 0.1f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { .start_offset_hz = 0.0f, .end_offset_hz = -600.0f, .duration = 0.1f }
    };

    // Configuração do Patch "Laser Pesado" (Camada extra para polifonia)
    MC_Patch heavy_laser_patch = {
        .wave = MC_WAVE_SAWTOOTH,
        .duty_cycle = 0.5f,
        .total_duration = 0.3f,
        .adsr = { .attack = 0.0f, .decay = 0.2f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { .start_offset_hz = 0.0f, .end_offset_hz = -800.0f, .duration = 0.2f }
    };

    printf("Teste de Disparo Unico...\n");
    printf("Pitiu! (880Hz -> 280Hz)\n");
    MC_Play(0, &laser_patch, 880.0f);
    MC_Wait(500);

    printf("Teste de Disparo Rapido (Burst)...\n");
    for(int i=0; i<5; i++) {
        MC_Play(0, &laser_patch, 880.0f);
        MC_Wait(120);
    }
    MC_Wait(500);

    printf("Teste de Polifonia: Double Shot Carregado...\n");
    // Canal 0: Laser Padrão
    // Canal 1: Laser Pesado (Brave)
    
    printf("Carregando...\n");
    MC_Play(0, &laser_patch, 1200.0f);
    MC_Wait(150);
    MC_Play(0, &laser_patch, 1500.0f);
    MC_Wait(150);
    
    printf("FOGO! (Stereo Beam)\n");
    MC_Play(0, &laser_patch, 880.0f);      // Laser principal
    MC_Play(1, &heavy_laser_patch, 440.0f); // Sub-harmônico pesado
    
    MC_Wait(1000);

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
