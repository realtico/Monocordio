#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Iniciando Protocolo Monocordio: Teste de Batalha (Polifonia)...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Falha crítica no sistema de áudio.\n");
        return 1;
    }

    // Camada 1: Ruído da Explosão (Textura)
    MC_Patch noise_layer = {
        .wave = MC_WAVE_NOISE_LFSR,
        .duty_cycle = 0.5f,
        .adsr = { .attack = 0.0f, .decay = 1.0f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { 0 },
        .volume = 0.8f
    };

    // Camada 2: Corpo da Explosão (Impacto Grave)
    MC_Patch body_layer = {
        .wave = MC_WAVE_SQUARE,
        .duty_cycle = 0.5f,
        .adsr = { .attack = 0.0f, .decay = 0.2f, .sustain = 0.0f, .release = 0.0f },
        .vfo = { 0 },
        .volume = 0.9f
    };

    // Alerta: Sirene Tática
    MC_Patch alert_patch = {
        .wave = MC_WAVE_SINE,
        .duty_cycle = 0.5f,
        .adsr = { .attack = 0.1f, .decay = 0.0f, .sustain = 0.6f, .release = 0.5f },
        .vfo = { 0 },
        .volume = 0.5f
    };

    printf("Combate Iniciado!\n");

    // Passo 1: Disparar Alerta (Canal 2)
    printf(">> Alerta Detectado (Canal 2: 880Hz)\n");
    MC_Play(2, &alert_patch, 880.0f);
    
    MC_Wait(500); // 0.5s de tensão

    // Passo 2: A Explosão Definitiva (Canais 0 + 1)
    printf(">> IMPACTO! (Canal 0: Ruído + Canal 1: Sub-Grave 50Hz)\n");
    MC_Play(0, &noise_layer, 220.0f); // Ruído sintonizado (220Hz dá um tom grave de textura)
    MC_Play(1, &body_layer, 50.0f);

    // Monitorar o decaimento
    MC_Wait(1500); 

    // Encerrar Alerta
    printf(">> Cessar Alerta.\n");
    MC_Stop(2);
    MC_Wait(1000); // Ouvir o release do alerta

    printf("Encerrando operações.\n");
    MC_Cleanup();
    return 0;
}
