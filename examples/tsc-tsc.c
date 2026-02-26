#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio: Teste de Ruido (LFSR)...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    // Configuração do Patch "Explosão" (MSX Style)
    // Envelope: Attack instantâneo (0.0s), Decay longo (1.5s), Sustain (0.0).
    // Objetivo: Ouvir a desintegração gradual do ruído metálico.
    MC_Patch explosion_patch = {
        .wave = MC_WAVE_NOISE_LFSR,
        .duty_cycle = 0.5f, // Irrelevante para ruído
        .total_duration = 2.0f, // Tempo suficiente para o decay
        .adsr = { .attack = 0.0f, .decay = 1.5f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { 0 }
    };

    // Configuração do Patch "Hi-Hat" (Prato de Bateria)
    // Envelope: Attack instantâneo (0.0s), Decay curtíssimo (0.05s), Sustain (0.0).
    // Objetivo: Um "tsh" seco e percussivo.
    MC_Patch hihat_patch = {
        .wave = MC_WAVE_NOISE_LFSR,
        .duty_cycle = 0.5f,
        .total_duration = 0.2f,
        .adsr = { .attack = 0.0f, .decay = 0.05f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { 0 }
    };

    printf("Teste 1: A Explosão (Canal 0)...\n");
    // A frequência base não afeta muito o LFSR neste implementação simples,
    // mas em alguns chips de som (como SN76489), a frequência do clock do shift register altera a textura.
    // Aqui, o LFSR roda update por sample, então a "frequência" é fixa na taxa de amostragem.
    MC_Play(0, &explosion_patch, 440.0f);
    
    // Aguardar o som terminar (Decay 1.5s)
    MC_Wait(2000);

    printf("Teste 2: Hi-Hat Rítmico (Canal 1)...\n");
    for (int i = 0; i < 4; i++) {
        printf("Tsh! ");
        fflush(stdout);
        MC_Play(1, &hihat_patch, 880.0f);
        MC_Wait(250); // 1/4 segundo entre hits
    }
    printf("\n");

    MC_Wait(500); // Wait for tail

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
