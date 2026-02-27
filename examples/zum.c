#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    printf("Sistema de Audio online. Preparando 'Plunct-Plact-Zum'...\n");

    // Configuração do Patch de Teste (O "Zum")
    MC_Patch square_wave_patch = {
        .wave = MC_WAVE_SQUARE,
        .duty_cycle = 0.5f,
        .total_duration = 2.0f, // 2 Segundos de ZUM
        .adsr = { .attack = 0.1, .decay = 0.1, .sustain = 0.8, .release = 0.2 },
        .vfo = { 0 },
        .volume = 0.5f
    };

    // Disparar Canal 0, 440Hz (A4)
    printf("Disparando Canal 0: Quadrada, 440Hz\n");
    MC_Play(0, &square_wave_patch, 440.0f);

    // Aguardar o som tocar (Smoke test rápido)
    MC_Wait(2500); // 2.5s

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
