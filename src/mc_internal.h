#ifndef MC_INTERNAL_H
#define MC_INTERNAL_H

#include "monocordio.h"
#include <SDL2/SDL.h>
#include <fluidsynth.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// CONSTANTS (GAINS)
// ============================================================================
#define MC_INTERNAL_GAIN 0.6f  // Ganho da engine 8-bit (baixado de 1.5 para equilibrar com o MIDI)
#define MC_FLUID_GAIN    0.6f  // Gain interno do FluidSynth (aumentado de 0.2 para maior presença)

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

// ADSR Stages
#define ADSR_IDLE    0
#define ADSR_ATTACK  1
#define ADSR_DECAY   2
#define ADSR_SUSTAIN 3
#define ADSR_RELEASE 4

/* Estado Dinâmico do Canal */
struct MC_Channel {
    bool active;
    double phase;
    float current_amplitude;
    uint8_t adsr_stage; // 0:IDLE, 1:ATTACK, 2:DECAY, 3:SUSTAIN, 4:RELEASE
    float stage_timer;
    float frequency;     // Stores the played frequency
    MC_Patch patch;      // Copy of the patch to avoid pointer issues
    uint32_t lfsr_reg;   // Estado para o ruído
    float vfo_timer;     // Timer independente para VFO
    double vibrato_phase;// Fase do LFO para Vibrato
    float volume;        // Volume do canal
    float release_start_level; // Amplitude no inicio do release

    /* Modo de Operação (Mundo 1 vs Mundo 2) */
    MC_ChannelMode mode; 
    int midi_last_note;  // Última nota MIDI tocada (para note-off)
    
    /* Estado do Sequenciador MML */
    const char* mml_cursor; // Ponteiro atual na string MML
    const char* mml_start;  // Inicio da string
    uint32_t mml_wait_samples; // Contador de espera para a proxima nota
    uint8_t mml_octave;     // Oitava atual (padrao 4)
    uint16_t mml_tempo;     // BPM (padrao 120)
    uint8_t mml_default_len;// Duracao padrao (padrao 4 = seminima)
    bool mml_active;        // Se o sequenciador esta rodando
};

// ============================================================================
// GLOBAL STATE (Shared across modules)
// ============================================================================

extern struct MC_Channel channels[MC_CHANNELS_COUNT];
extern SDL_AudioDeviceID audio_device;
extern float master_volume;

// FluidSynth State
extern fluid_settings_t* fluid_settings;
extern fluid_synth_t* fluid_synth;
extern bool fluid_enabled;

// ============================================================================
// INTERNAL FUNCTION PROTOTYPES
// ============================================================================

// mc_midi.c
bool MC_Internal_InitFluidSynth(void);
void MC_Internal_CleanupFluidSynth(void);

// mc_mml.c
void MC_Internal_ProcessMML(struct MC_Channel* channel);

#endif // MC_INTERNAL_H
