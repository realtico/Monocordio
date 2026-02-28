# Parecer do Claudio v2 — Ombudsman do Código

**Data:** 27/02/2026  
**Escopo:** Delta desde o Review v1 (26/02). Avaliação da reorganização, novas features (Vibrato, MIDI Bridge, IPC Player) e correções implementadas.

---

## Scorecard: O Que Foi Corrigido do Review v1

| Item do Review v1 | Status | Nota |
|:---|:---:|:---|
| `MC_Channel` exposto no header | **CORRIGIDO** | Agora opaco via `typedef struct MC_Channel MC_Channel;` no header público, definição real em `mc_internal.h`. Exatamente o que pedi. |
| Race condition MC_Play + MML | **CORRIGIDO** | `MC_Play` agora faz `ch->mml_active = false;` antes de configurar o canal. Perfeito. |
| LFSR com seed fixo | **CORRIGIDO** | Agora usa `(rand() % 0xFFFF) ^ 0xACE1u` com `srand(time(NULL))` no init. Funcional. |
| LFSR clocked pela frequência | **CORRIGIDO** | O noise agora usa o `phase` accumulator para clockar o shift register. Implementação fiel à sugestão. |
| Falta `MC_IsPlaying()` | **CORRIGIDO** | Implementado. Retorna `active || adsr_stage != IDLE`. |
| Falta volume por canal | **CORRIGIDO** | `MC_SetVolume()` + campo `volume` no canal + `volume` no `MC_Patch`. Duplo controle. |
| Falta `MC_SetMasterVolume()` | **CORRIGIDO** | Implementado com clamp em 0.0. |
| Hard clipping na mixagem | **CORRIGIDO** | Agora usa `tanhf()` (soft clip). |
| README com path `libs/monocordio/` errado | **NÃO CORRIGIDO** | README ainda diz `libs/monocordio/`. A estrutura real é `src/` + `include/`. |
| Pasta `patches/` vazia | **NÃO CORRIGIDO** | Continua lá, vazia. |

**Saldo: 8/10 corrigidos.** Os 2 restantes são cosméticos mas continuam incomodando.

---

## 1. A REORGANIZAÇÃO DO CÓDIGO

A decomposição de `monocordio.c` monolítico em 5 módulos foi bem executada:

| Módulo | Responsabilidade | Veredicto |
|:---|:---|:---|
| `mc_core.c` | AudioCallback, Init/Cleanup, MC_Play/Stop, estado global | OK — é o "kernel" |
| `mc_synth.c` | Helpers de síntese (GetNoteFreq) | **Anêmico** — só tem 1 função de 3 linhas |
| `mc_mml.c` | Parser MML completo | OK — bem isolado |
| `mc_midi.c` | FluidSynth init, API MIDI, IPC Player | OK — boa separação |
| `mc_presets.c` | Banco de patches | OK — dados puros |
| `mc_internal.h` | Contrato interno entre módulos | OK — bom padrão |

### Problemas:

**`mc_synth.c` é um módulo fantasma.** Tem 20 linhas incluindo comentários. O comentário dentro dele *já se desculpa* por não fazer mais: "*the actual waveform generation loop is currently tightly coupled into AudioCallback*". Ou movam a geração de waveform para cá (o correto), ou absorvam `GetNoteFreq` no `mc_mml.c` (onde é usado) e deletem o arquivo. Módulo de 1 função é ruído organizacional.

**O AudioCallback faz I/O de pipe (`read()`) dentro do callback de áudio.** Isso está em `mc_core.c` linhas 43-82. O callback de áudio da SDL roda numa thread de alta prioridade com deadline rígido (~23ms). Fazer `read()` de pipe — mesmo non-blocking — dentro dele é **perigoso**. Se o kernel precisar de um page fault no buffer, ou se o pipe tiver mais dados que cabem em 128 bytes e exigir múltiplas iterações, vocês podem estourar o deadline e gerar dropouts audíveis.

**Sugestão:** Mover a leitura do pipe para uma thread auxiliar dedicada que lê do pipe e faz `MC_SendRawMidi()`. O callback de áudio só deveria gerar amostras e mixar. Isso é princípio sagrado em áudio: *"a callback de áudio não faz I/O, não aloca memória, não toca em locks de longa duração"*.

---

## 2. O MIDI BRIDGE (Mundo 2 + 3)

Essa é a feature mais ambiciosa e a que mais precisa de atenção.

### O que está bom:
- A API de conveniência MIDI (`MC_MidiNoteOn`, `MC_MidiProgramChange`, etc.) é excelente. Encapsula o protocolo MIDI raw de forma limpa.
- O `MC_MidiAllNotesOff()` como "panic button" é essencial e está implementado corretamente.
- O conceito de `MC_MODE_INTERNAL` vs `MC_MODE_EXTERNAL` por canal é elegante.

### O que precisa de atenção:

**O `MC_Cleanup()` não limpa o IPC.** Se o usuário chama `MC_PlayMidiFile()` e depois `MC_Cleanup()`, o processo filho continua rodando como órfão (zombie). O pipe fd[0] nunca é fechado. Isso é um **resource leak real**. `MC_Cleanup()` precisa:
```c
void MC_Cleanup(void) {
    // Kill IPC player if active
    if (mc_ipc_active && mc_player_pid > 0) {
        kill(mc_player_pid, SIGTERM);
        waitpid(mc_player_pid, NULL, 0);
        close(mc_pipe_fd[0]);
        mc_ipc_active = false;
    }
    SDL_PauseAudioDevice(audio_device, 1);
    SDL_CloseAudioDevice(audio_device);
    MC_Internal_CleanupFluidSynth();
    SDL_Quit();
}
```

**O parser MIDI no processo filho é frágil.** Ele só lê a **primeira track** e depois faz `break`. Para MIDI Type 0 (single track), funciona. Para Type 1 (multi-track, o formato mais comum), descarta todas as tracks exceto a primeira. O "Never Gonna Give You Up" provavelmente é Type 1 — vocês estão ouvindo só uma fração da música.

Sem contar que o parser não tem tratamento de erro robusto: se o arquivo estiver truncado ou corrompido, o `fseek` pode ir para posições inválidas sem checkagem de `ftell` vs file size.

**O FluidSynth render no callback tem um bug sutil.** Em `mc_core.c`:
```c
int frames = len / sizeof(float); // "This is total floats in mono buffer"
```
Mas logo abaixo:
```c
fluid_synth_write_float(fluid_synth, frames, fluid_buf, 0, 2, fluid_buf, 1, 2);
```
Vocês pedem ao FluidSynth para renderizar `frames` **frames** com stride 2 (interleaved stereo) — isso requer `frames * 2` floats no buffer. Mas `frames` aqui é o número de **samples mono** da SDL. Se `len = 1024 * sizeof(float)`, então `frames = 1024`, e vocês precisam de `2048` floats no `fluid_buf`. O buffer estático é `float fluid_buf[2048]`, então funciona *por acaso* para 1024 samples. Mas o clamp `if (frames > 1024) frames = 1024;` deveria ser `if (frames > 2048) frames = 1024;` ou melhor, `frames = len / sizeof(float)` que já é o correto para mono... na verdade, re-lendo: `frames` para `fluid_synth_write_float` é o número de **frames** (amostras por canal), e stride 2 com buffer size 2048 suporta no máximo 1024 frames. Está correto numericamente, mas a variável `samples` no loop principal e `frames` aqui são **a mesma coisa** — nomes diferentes para o mesmo valor. Renomear para clareza seria saudável.

**O ganho do FluidSynth é multiplicado por 2.0f duas vezes:** uma no `fluid_synth_set_gain(2.0f)` e outra no mixing `fluid_mono * 2.0f`. Resultado: ganho efetivo 4x sobre o normal. Junto com o `master_volume * 1.5f` no synth interno, a cadeia de ganho é um campo minado. Recomendo um diagrama de signal flow com os ganhos anotados para não perder o controle.

**`MC_SendSysEx` está declarado no header mas não implementado.** O comentário diz "Future Use" — ok, mas um header público com função declarada sem corpo vai causar linker error em quem tentar chamar. Ou implementem um stub (`void MC_SendSysEx(...) { /* TODO */ }`) ou removam da API pública até estar pronto.

---

## 3. BUGS E PROBLEMAS CONCRETOS

### Bug: `duty_cycle` morreu silenciosamente
No `MC_Patch` ainda existe o campo `duty_cycle`. Os presets ainda definem valores como `0.25f` (órgão) e `0.1f` (metal attack). **Mas o gerador de onda quadrada no AudioCallback agora faz:**
```c
case MC_WAVE_SQUARE:
    sample_val = (sin(ch->phase) >= 0.0) ? 1.0f : -1.0f;
```
Isso é duty cycle fixo de 50%, sempre. O campo `duty_cycle` **é completamente ignorado**. O órgão de 25% que deveria soar "oco" agora soa idêntico ao clarinete de 50%. A versão anterior usava `ch->phase < (2.0 * M_PI * patch->duty_cycle)` — isso precisa voltar:
```c
case MC_WAVE_SQUARE: {
    double norm = ch->phase / (2.0 * M_PI);
    norm -= (int)norm; // normalizar 0..1
    sample_val = (norm < ch->patch.duty_cycle) ? 1.0f : -1.0f;
    break;
}
```

### Bug: `MC_Play(0, &MC_PATCH_ORGAN, 0)` ainda acontece no orchestra.c
Linhas 35-36 do orchestra.c fazem `MC_Play(0, &MC_PATCH_ORGAN, 0)` e `MC_Play(1, &MC_PATCH_BASS, 0)` — frequência zero. Isso gera canais ativos com fase que nunca avança. O bug do review v1 **continua lá**. A lib deveria rejeitar freq <= 0 (return early) ou o exemplo precisa ser corrigido.

### Bug: Numeração dos presets no header vs implementation
O `mc_presets.h` usa uma numeração nova (VIOLIN = 10, KICK = 11, SNARE = 12...) mas `mc_presets.c` **ainda tem os comentários antigos** (KICK = "10: Kick", SNARE = "11: Snare"). Isso confunde quem lê o `.c` depois de ler o `.h`.

### Bug: Release rate não foi corrigido
Eu pedi para guardar `release_start_level` para calcular a taxa de decaimento proporcional. A implementação atual ainda faz:
```c
float rate = 1.0f / ch->patch.adsr.release;
ch->current_amplitude -= rate * dt;
```
O comentário diz "Release formula fix based on max amplitude assumption for rate" — mas é exatamente o mesmo bug: se o sustain é 0.3 e o release é 0.5s, a amplitude leva `0.3 / (1.0/0.5) = 0.15s` para chegar a zero, quando deveria levar 0.5s inteiros. É um release 3x mais rápido que o configurado.

---

## 4. DOCUMENTAÇÃO

O MANUAL foi atualizado com as APIs MIDI e está bem estruturado. Os problemas remanescentes:

- **README:** Estrutura do projeto ainda lista `libs/monocordio/` (não existe) em vez de `src/` + `include/`. Também não menciona nenhum dos novos exemplos (`metal_violin`, `midi_hybrid`). Só lista até `samsung`.
- **MANUAL:** Diz `channel: 0 a 15` para `MC_Play`, mas `MC_CHANNELS_COUNT` é 8 (0 a 7). 15 seria o range MIDI, não o range da engine interna.
- **MANUAL:** Ainda referencia `libs/monocordio/monocordio.h` no título da seção de API.
- **A fórmula de MML->Hz no MANUAL** diz `n-49` mas a implementação usa `midi_note - 69`. Ambas são corretas (é a mesma fórmula com indexações diferentes), porém a doc deveria refletir o código, não a versão alternativa.
- **Faltam docs para Vibrato.** Os campos `vibrato_rate` e `vibrato_depth` no `MC_Patch` não aparecem no MANUAL.

---

## 5. ARQUITETURA & SABEDORIA

### O Elefante na Sala: FluidSynth como Dependência Obrigatória

O `MC_Init()` tenta inicializar o FluidSynth e, se falhar, imprime um warning e continua. Isso é o comportamento correto. **Mas** o Makefile compila tudo com `-lfluidsynth` obrigatório. Se alguém quiser usar o Monocordio *só* no modo 8-bit (o caso de uso original!), precisa ter FluidSynth instalado mesmo sem usá-lo.

Sugestão: um `make MIDI=0` que compila sem FluidSynth, usando `#ifdef MC_MIDI_ENABLED` no código. Isso preserva o caráter minimalista original e faz FluidSynth ser opt-in.

### Signal Flow não documentado

A cadeia atual de ganho é:
```
sample * envelope * channel_volume * patch_volume  →  * master_volume * 1.5  →  + fluid_mono * 2.0  →  tanhf()
```
Esse `* 1.5f` hardcoded no final do loop interno é um número mágico sem comentário. O `* 2.0f` no fluid mix é outro. Juntos, tornam o balanceamento entre Mundo 1 (8-bit) e Mundo 2 (FluidSynth) dependente de ajuste manual oculto. Um `#define MC_INTERNAL_GAIN 1.5f` e `#define MC_FLUID_GAIN 2.0f` no `mc_internal.h` ajudaria a manter a sanidade.

### O `total_duration` continua no `MC_Patch` mas não é processado

Grepping `total_duration` em `mc_core.c` retorna **zero resultados**. O campo existe no struct, os presets definem ele (kick: 0, splash: 0), mas o AudioCallback simplesmente não o usa mais. Isso é código morto que confunde. Ou reimplementem o mecanismo ou removam o campo.

### IPC via `fork()` + `pipe()` é válido, mas frágil

Funciona para a demo (tocar um .mid), mas:
- Não há como parar o player sem matar o processo (`MC_StopMidiFile()` não existe).
- Se o processo filho crashar (MIDI corrompido), o pai não sabe — `mc_ipc_active` continua `true`.
- Não é portável para Windows (sem `fork`).

Para o escopo atual (demo), está ok. Para produção, considere usar `fluid_player` direto no mesmo processo (FluidSynth já tem um player MIDI embutido!) — eliminaria todo o código de IPC.

---

## 6. NOVOS EXEMPLOS

**`metal_violin.c`** — Excelente demo. O vibrato no violino é audível e natural. A percussão layered (metal + noise) mostra o potencial da engine. Bom showcase.

**`midi_hybrid.c`** — Conceitualmente forte (8-bit + MIDI lado a lado), mas o `sleep(1)` no início é um hack. Se FluidSynth precisa de tempo para iniciar, isso deveria ser tratado internamente (e.g., aguardar o SoundFont carregar antes de retornar de `MC_Init()`).

**`orchestra.c`** — Não foi atualizado desde v1. Ainda tem os bugs de `MC_Play(..., 0)` e `MC_Play(..., &MC_PATCH_SPLASH, 0)` (freq 0 para ruído e splash). Agora que a frequência controla o clock do LFSR, `freq=0` para ruído significa "LFSR nunca clocka" = silêncio ou DC. Isso é bug funcional.

---

## Veredicto v2

A evolução em 24h é impressionante. O projeto saltou de "engine de síntese 8-bit" para "plataforma de áudio híbrida com dois motores". A amplitude da ambição cresceu, e com ela, a superfície de bugs.

### As 3 mudanças de maior impacto agora:
1. **Restaurar `duty_cycle` no gerador de onda quadrada** — é regressão, e mata a identidade sonora do órgão e metal
2. **Limpar IPC no `MC_Cleanup()` + tratar `total_duration` (remover ou reimplementar)** — resource leaks e código morto
3. **Tirar I/O (`read()`) de dentro do AudioCallback** — bomba-relógio de latência

### Resumo por eixo:

| Eixo | v1 | v2 | Tendência |
|:---|:---:|:---:|:---:|
| **Clareza** | 7/10 | 7/10 | → (docs não acompanharam as mudanças) |
| **Estabilidade** | 6/10 | 7/10 | ↑ (fixes do review v1, mas novos bugs de regressão) |
| **UX** | 6/10 | 8/10 | ↑↑ (MC_IsPlaying, volumes, MIDI API) |
| **Ambição** | 7/10 | 9/10 | ↑↑ (Mundo 1+2+3 é audacioso) |

**Nota do Claudio: 7.5/10 → 8.0/10** — A fundação ficou mais forte e a visão se expandiu. Mas a velocidade de desenvolvimento introduziu regressões que precisam ser caçadas. A dica de ouro: **depois de cada sprint, rodem todos os exemplos antigos antes de commitar. Se o "zum" e o "pling" ainda soam certo, o resto provavelmente está ok.**

---

*"Rápido, barato, bom — escolha dois." Vocês estão escolhendo rápido e bom. Cuidado para não pagar o preço no terceiro.*

— Claudio, Ombudsman
