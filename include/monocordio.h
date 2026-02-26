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
} MC_Patch;

/* Estado Dinâmico do Canal */
typedef struct {
    bool active;
    double phase;
    float current_amplitude;
    uint8_t adsr_stage; // 0:IDLE, 1:ATTACK, 2:DECAY, 3:SUSTAIN, 4:RELEASE
    float stage_timer;
    float frequency;     // Stores the played frequency
    const MC_Patch* patch;
    uint32_t lfsr_reg;  // Estado para o ruído
    float vfo_timer;    // Timer independente para VFO

    /* Estado do Sequenciador MML */
    const char* mml_cursor; // Ponteiro atual na string MML
    const char* mml_start;  // Inicio da string (para loops se precisar, ou debug)
    uint32_t mml_wait_samples; // Contador de espera para a proxima nota
    uint8_t mml_octave;     // Oitava atual (padrao 4)
    uint16_t mml_tempo;     // BPM (padrao 120)
    uint8_t mml_default_len;// Duracao padrao (padrao 4 = seminima)
    bool mml_active;        // Se o sequenciador esta rodando
} MC_Channel;

/* API Principal */
bool MC_Init(void);
void MC_Cleanup(void);
void MC_Play(int channel_id, const MC_Patch* patch, float base_freq);
void MC_PlayMML(int channel_id, const MC_Patch* patch, const char* mml_string);
void MC_Stop(int channel_id);
void MC_Wait(uint32_t ms);

#endif // MONOCORDIO_H
