# 📖 Manual Técnico do Monocordio

Este documento detalha a API, as estruturas de dados e a linguagem de script musical (MML) do Monocordio.

## � Uso como Biblioteca Dinâmica (.so)

Após instalar a biblioteca (`sudo make install`), você pode compilar seus projetos linkando com o **Monocordio**:

```bash
gcc meu_jogo.c -o meu_jogo -lmonocordio -lSDL2 -lm
```

Certifique-se de incluir o header no seu código:

```c
#include <monocordio.h>
```

---

## 🛠️ API C (include/monocordio.h)

### Controle do Sistema

| Função | Descrição |
| :--- | :--- |
| `bool MC_Init(void)` | Inicializa a SDL2 e o dispositivo de áudio. Retorna `true` se sucesso. |
| `void MC_Cleanup(void)` | Encerra a SDL2 e libera recursos. Deve ser chamada ao fim do programa. |
| `void MC_SetMasterVolume(float vol)` | Define o volume global de toda a mix. 0.0 a N (default 0.3). |
| `void MC_SetVolume(int channel, float vol)` | Define o ganho individual do canal (0.0 a 2.0). |
| `bool MC_IsPlaying(int channel)` | Retorna `true` se o canal estiver soando (ativo ou em release). |

### Reprodução Sonora (Engine 8-Bit)

| Função | Descrição |
| :--- | :--- |
| `void MC_Play(int channel, const MC_Patch* patch, float freq)` | Dispara um som **imediato** no canal especificado. <br>• `channel`: 0 a 7.<br>• `patch`: Ponteiro para a configuração do timbre.<br>• `freq`: Frequência base em Hz. |
| `void MC_Stop(int channel)` | Interrompe o som no canal. Se o Patch tiver release, ele entra na fase de Release; caso contrário, corta imediatamente. |
| `void MC_PlayMML(int channel, const MC_Patch* patch, const char* mml)` | Inicia o **Sequenciador MML** no canal. O canal irá interpretar a string `mml` em tempo real, background. |

---

## 🎹 MIDI Bridge (Hybrid Engine) - Sprint 09

O Monocordio agora possui integração completa com **FluidSynth**, permitindo sons sampleados profissionais mixados com a síntese 8-bit.

### Modos de Canal (MC_ChannelMode)
Cada canal pode operar em dois universos distintos:
*   `MC_MODE_INTERNAL` (Padrão): O canal gera onda bruta (SQUARE, SAW, NOISE).
*   `MC_MODE_EXTERNAL`: O canal envia comandos para o sintetizador MIDI (banco GM).

```c
MC_SetChannelMode(channel, MC_MODE_EXTERNAL);
```

### Comandos de Conveniência MIDI
Use estas funções para controlar instrumentos reais no modo externo:

| Função | Descrição |
| :--- | :--- |
| `MC_MidiNoteOn(ch, note, vel)` | Toca uma nota MIDI (0-127). |
| `MC_MidiNoteOff(ch, note)` | Para uma nota MIDI. |
| `MC_MidiProgramChange(ch, patch)` | Troca o instrumento do canal (0-127). Ex: 0=Piano, 29=Overdrive Guitar. |
| `MC_MidiControlChange(ch, cc, val)` | Envia um Control Change genérico. |
| `MC_MidiSetVolume(ch, vol)` | Helper para CC 7 (Volume do Instrumento). |
| `MC_MidiAllNotesOff()` | Envia comando de silêncio para todos os canais (Panic). |

---

## 🎛️ Estruturas de Dados (O Patch)

O som no Monocordio é definido pelo struct `MC_Patch`. Ele é composto por três módulos principais:

### 1. Forma de Onda (`MC_Waveform`)
Define a textura básica do som.
*   `MC_WAVE_SQUARE`: Som de "gameboy", cortante. (Usa `duty_cycle`).
*   `MC_WAVE_TRIANGLE`: Suave, similar a uma flauta ou baixo sutil.
*   `MC_WAVE_SAWTOOTH`: Agressivo, rico em harmônicos (metais, cordas).
*   `MC_WAVE_SINE`: Som puro, "limpo".
*   `MC_WAVE_NOISE_LFSR`: Ruído pseudo-aleatório (explosões, percussão).

### 2. Envelope (`MC_ADSR`)
Controla a evolução do volume no tempo.
*   **Attack:** Tempo (s) para ir do silêncio ao volume máximo.
*   **Decay:** Tempo (s) para cair do máximo ao Sustain.
*   **Sustain:** Nível constante (0.0 a 1.0) enquanto a nota soa.
*   **Release:** Tempo (s) para silenciar após o `MC_Stop`.

### 3. Oscilador de Frequência Variável (`MC_VFO`)
Permite alterar a frequência durante a reprodução (Pitch Slide).
*   `start_offset_hz`: Desvio de frequência no início.
*   `end_offset_hz`: Desvio de frequência no final.
*   `duration`: Tempo para percorrer do início ao fim.

### 4. LFO / Vibrato
Permite modulação periódica da frequência.
*   `vibrato_rate`: Velocidade do vibrato em Hz (ex: `5.0f`).
*   `vibrato_depth`: Quantidade de modulação/desvio da frequência original (ex: `5.0f`).

---

## 🎼 Dicionário MML (Music Macro Language)

O Monocordio possui um interpretador MML embutido para tocar melodias. As comandos são processados sequencialmente.

### Tabela de Comandos

| Comando | Significado | Exemplo | Descrição |
| :--- | :--- | :--- | :--- |
| **A - G** | Notas Musicais | `C`, `E`, `G#` | Toca a nota correspondente. <br>Sufixos: `#` ou `+` (Sustenido), `-` (Bemol). |
| **0 - 9** | Duração da Nota | `C4`, `D8` | Define a duração da nota específica. <br>4 = Semínima, 8 = Colcheia, 16 = Semicolcheia, etc. |
| **R** | Rest (Pausa) | `R4`, `R8` | Pausa (silêncio) pela duração especificada. |
| **O** | Octave (Oitava) | `O4`, `O5` | Define a oitava absoluta (0 a 8). Padrão é 4. |
| **>** | Octave Up | `>` | Sobe uma oitava relativo à atual. |
| **<** | Octave Down | `<` | Desce uma oitava relativo à atual. |
| **L** | Default Length | `L8` | Define a duração padrão para as próximas notas que não tiverem número explícito. |
| **T** | Tempo (BPM) | `T120` | Define a velocidade da música em Batidas Por Minuto. |

### Exemplo de String MML
```c
// Escala de Dó Maior Rápida (Tempo 180)
const char* escala = "T180 O4 C D E F G A B > C";
```

### Curiosidade Técnica: Conversão MML -> Hz
O Monocordio converte as notas musicais para frequência utilizando a fórmula padrão da temperamento igual:

$$f = 440 \cdot 2^{\frac{n-69}{12}}$$

Onde $n$ é o índice da nota MIDI (Sendo A4 = 69 = 440Hz). O parser calcula isso em tempo real dentro do loop de áudio para garantir precisão máxima de afinação.
