#ifndef MONOCORDIO_H
#define MONOCORDIO_H

#include <stdint.h>
#include <stdbool.h>

/* Configurações Globais */
#define MC_CHANNELS_COUNT 8
#define MC_SAMPLE_RATE 44100

/* Definições de Onda */
typedef enum {
    MC_WAVE_SQUARE,
    MC_WAVE_TRIANGLE,
    MC_WAVE_SAWTOOTH,
    MC_WAVE_SINE,
    MC_WAVE_NOISE_LFSR
} MC_Waveform;

/* Envelopes */
typedef struct {
    float attack;  // Tempo (s)
    float decay;   // Tempo (s)
    float sustain; // Nível (0.0 a 1.0)
    float release; // Tempo (s)
} MC_ADSR;

typedef struct {
    float start_offset_hz;
    float end_offset_hz;
    float duration;
} MC_VFO;

/* Template de Som */
typedef struct {
    MC_Waveform wave;
    MC_ADSR adsr;
    MC_VFO vfo;
    float total_duration;
    float duty_cycle; // Relevante para SQUARE
    float vibrato_depth; // Amplitude do Vibrato (Hz)
    float vibrato_rate;  // Frequência do Vibrato (Hz)
    float volume;     // Novo: Controle de ganho por patch (mastering)
} MC_Patch;

/* Estado Dinâmico do Canal (Opaco) */
typedef struct MC_Channel MC_Channel;

/* API Principal */
bool MC_Init(void);
void MC_Cleanup(void);
void MC_Play(int channel_id, const MC_Patch* patch, float base_freq);
void MC_PlayMML(int channel_id, const MC_Patch* patch, const char* mml_string);
void MC_Stop(int channel_id);
void MC_Wait(uint32_t ms);

/* Controle e Qualidade (Novas Funções do Polux) */
bool MC_IsPlaying(int channel_id);
void MC_SetVolume(int channel_id, float vol);
void MC_SetMasterVolume(float vol);

#endif // MONOCORDIO_H
