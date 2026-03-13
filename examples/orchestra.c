/*
 * orchestra.c - Exemplo de uso do Banco de Presets (GM16) e Polifonia
 * Toca o tema de "O Fantasma da Opera"
 */

#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"
#include "mc_presets.h"

int main() {
    printf("Iniciando Monocordio Orchestra...\n");

    if (!MC_Init()) {
        fprintf(stderr, "Erro ao inicializar audio.\n");
        return 1;
    }

    printf("Carregando instrumentos...\n");
    // Canal 0: Órgão (Melodia Principal)
    // Canal 1: Strings (Bass/Pad)
    // Canal 2: Percussão (Kick/Snares via MML commands se possível, ou triggers manuais)
    // Canal 3: Arpeggio Fantasma (Flute)

    // O MML do Monocordio é simples. Vamos usar Play normais para demonstrar a polifonia controlada 
    // ou uma string MML complexa se a engine suportar múltiplos canais via string única (não suporta, é por canal).

    // Tema: Phantom of the Opera (Chromatic Descent)
    // D C# C B Bb

    // Definindo Patches
    // Canal 0: O icônico Órgão
    // MC_Play(0, &MC_PATCH_ORGAN, 0); 
    // Canal 1: Baixo de suporte
    // MC_Play(1, &MC_PATCH_BASS, 0);

    // Frase 1: Taaaaaaan...
    printf("Phrase 1: The Phantom...\n");
    // Organ D4
    MC_Play(0, &MC_PATCH_ORGAN, 587.33f); // D5
    
    // Bass D2
    MC_Play(1, &MC_PATCH_BASS, 146.83f); // D3
    
    // Kick e Crash
    MC_Play(2, &MC_PATCH_KICK, 60.0f);
    MC_Play(3, &MC_PATCH_SPLASH, 4400.0f);
    
    MC_Wait(800);

    // Chromatic Run Down
    float melody[] = {
        554.37f, // C#5
        523.25f, // C5
        493.88f, // B4
        466.16f  // Bb4
    };

    for(int i=0; i<4; i++) {
        MC_Play(0, &MC_PATCH_ORGAN, melody[i]);
        MC_Play(2, &MC_PATCH_HH_CLOSED, 8000.0f); // Metronome click
        MC_Wait(250);
    }
    
    // Long Note Low (Bb)
    MC_Play(1, &MC_PATCH_BASS, 233.08f); // Bb3
    MC_Play(2, &MC_PATCH_KICK, 60.0f);
    MC_Play(3, &MC_PATCH_EXPLOSION_SUB, 80.0f); // Impacto dramático
    MC_Wait(1000);
    
    // Phrase 2: Repetição mais grave
    printf("Phrase 2: Inside your mind...\n");
    
    MC_Play(0, &MC_PATCH_ORGAN, 293.66f); // D4
    MC_Play(2, &MC_PATCH_SNARE, 5000.0f);
    MC_Wait(800);

    float melody2[] = {
         277.18f, // C#4
         261.63f, // C4
         246.94f, // B3
         233.08f  // Bb3
    };

    for(int i=0; i<4; i++) {
        MC_Play(0, &MC_PATCH_ORGAN, melody2[i]);
        MC_Play(2, &MC_PATCH_HH_OPEN, 8000.0f);
        MC_Wait(250);
    }

    // Finale
    MC_Play(0, &MC_PATCH_ORGAN, 146.83f); // D3 Low
    MC_Play(1, &MC_PATCH_STRINGS, 146.83f); // Strings D3
    MC_PlayExplosion(4, 5); // Canais 4 e 5 explodem
    
    printf("Grand Finale...\n");
    MC_Wait(3000);

    MC_Cleanup();
    printf("Fim da Performance.\n");
    return 0;
}
