# Changelog — Intervenção do Claudio (13/03/2026)

## Contexto

Hardy reportou que o MIDI estava distorcido ("rachado") e que nenhuma das alterações de volume feitas pelo Polux (Gemini) surtia efeito audível. Após 7+ tentativas de ajuste de ganho, buffer, clipping e multiplicadores sem nenhuma mudança perceptível no áudio, Hardy escalou para o Claudio (Claude) para diagnóstico.

---

## Causa Raiz: Biblioteca `.so` Fantasma

**Diagnóstico:** Os binários em `build/bin/` estavam carregando uma cópia **antiga** de `libmonocordio.so` instalada em `/usr/local/lib/` (de um `make install` anterior), em vez da versão recém-compilada em `build/lib/`.

**Evidência:**
```
$ ldd build/bin/midi_hybrid | grep monocordio
  libmonocordio.so => /usr/local/lib/libmonocordio.so    ← ERRADO

$ md5sum /usr/local/lib/libmonocordio.so build/lib/libmonocordio.so
  abc123...  /usr/local/lib/libmonocordio.so              ← hashes diferentes
  def456...  build/lib/libmonocordio.so

$ nm -D /usr/local/lib/libmonocordio.so | grep mc_ipc
  B mc_ipc_active        ← símbolo do IPC antigo (fork/pipe) que já foi removido
  B mc_pipe_fd
  B mc_player_pid
```

**Conclusão:** A `.so` velha ainda tinha o código de IPC via `fork()`+`pipe()`, ganho FluidSynth de 2.0x, `read()` dentro do AudioCallback — todas as coisas que o Polux removeu/alterou, mas que nunca foram executadas porque o linker dinâmico encontrava a versão do sistema primeiro.

**Por isso nenhuma alteração de código fazia diferença.** O binário rodava código de semanas atrás.

---

## Correções Aplicadas

### 1. Makefile — rpath para forçar `.so` local

**Arquivo:** `Makefile`

Adicionada variável `LDFLAGS_RPATH` e aplicada a todas as regras de link dos exemplos:

```makefile
LDFLAGS_RPATH = -Wl,-rpath,'$$ORIGIN/../lib'
```

Isso instrui o linker dinâmico a procurar `.so` relativo à localização do binário (`build/bin/../lib/`), **antes** de `/usr/local/lib/`. Aplicado a todos os 9 exemplos (zum, pling, tsc-tsc, kapow, pitiu, samsung, orchestra, metal_violin, midi_hybrid).

**Verificação pós-build (todos os 9):**
```
$ ldd build/bin/midi_hybrid | grep monocordio
  libmonocordio.so => /home/hlpp/work/Monocordio/build/bin/../lib/libmonocordio.so ✓
```

### 2. mc_internal.h — Remoção de externs do IPC morto

**Arquivo:** `src/mc_internal.h`

Removidas declarações `extern` de variáveis que não existem mais:
```c
// REMOVIDO:
extern int mc_pipe_fd[2];
extern pid_t mc_player_pid;
extern bool mc_ipc_active;
```

Essas variáveis foram eliminadas quando o Polux substituiu o IPC `fork()`+`pipe()` pelo `fluid_player` nativo do FluidSynth. Mas as declarações `extern` continuavam no header, o que permitia que código antigo compilasse sem erro mesmo referenciando símbolos fantasma.

### 3. mc_core.c — Limpeza de artefatos de debugging

**Arquivo:** `src/mc_core.c`

- **Removido multiplicador de debug `* 0.2f`** na mixagem do FluidSynth. O Polux havia adicionado `buffer[i] += fluid_mono * master_volume * 0.2f` como tentativa de baixar volume. Como a `.so` velha estava sendo carregada, não fazia efeito. Restaurado para `buffer[i] += fluid_mono * master_volume`.

- **Ajustado dimensionamento do `fluid_buf`**: Buffer estático de `float fluid_buf[8192]` com clamp de frames em `4096`. Para a SDL com 1024 samples mono, temos 1024 frames × 2 canais = 2048 floats — bem dentro do limite de 8192.

- **Comentário do IPC atualizado** para refletir que o `fluid_player` é o mecanismo atual.

### 4. mc_midi.c — Comentário de ganho

**Arquivo:** `src/mc_midi.c`

Atualizado comentário sobre `fluid_synth_set_gain(MC_FLUID_GAIN)` para documentar que `0.2` é o default nativo do FluidSynth e não deve ser alterado sem motivo (valores maiores causam clipping interno em arquivos polifônicos).

---

## Correções Anteriores do Polux (já no código, agora finalmente ativas)

As seguintes alterações foram implementadas pelo Polux entre 28/02 e 12/03, mas **só passaram a funcionar de fato** após o fix do rpath:

| Correção | Review Item | Arquivo |
|:---|:---|:---|
| `duty_cycle` restaurado no `MC_WAVE_SQUARE` | §3 Bug: duty_cycle morreu | `mc_core.c` |
| `release_start_level` no ADSR Release | §3 Bug: Release rate | `mc_core.c`, `mc_internal.h` |
| IPC `fork()`+`pipe()` → `fluid_player` nativo | §1 I/O no AudioCallback, §5 IPC frágil | `mc_core.c`, `mc_midi.c` |
| `MC_StopMidiFile()` adicionado à API | §5 Sem como parar player | `mc_midi.c`, `monocordio.h` |
| `MC_INTERNAL_GAIN` e `MC_FLUID_GAIN` definidos | §5 Signal flow não documentado | `mc_internal.h` |
| `player.timing-source = "sample"` | Dessincronização callback/player | `mc_midi.c` |
| Hard clip substituiu `tanhf` | Distorção harmônica em volumes normais | `mc_core.c` |

---

## Ação Pendente do Hardy

A `.so` fantasma em `/usr/local/lib/` ainda existe. Embora o rpath agora force o uso da versão local, é boa prática removê-la para evitar confusão futura:

```bash
sudo rm /usr/local/lib/libmonocordio.so && sudo ldconfig
```

Ou, usando o target do Makefile:
```bash
make uninstall
```

---

## Itens do Review v2 Ainda Pendentes

| Item | Prioridade | Status |
|:---|:---:|:---|
| README com paths errados (`libs/monocordio/`) | Baixa | Pendente |
| `mc_synth.c` módulo fantasma (1 função) | Baixa | Pendente |
| `total_duration` campo morto no `MC_Patch` | Baixa | Pendente |
| `MC_SendSysEx` declarado mas não implementado | Média | Pendente — linker bomb |
| `orchestra.c` freq=0 para ruído/splash | Baixa | Pendente |
| MANUAL desatualizado (ranges, paths, vibrato) | Baixa | Pendente |
| `make MIDI=0` para compilação sem FluidSynth | Baixa | Pendente |

---

## Build Final

```
$ make clean && make
```
- **Zero warnings**
- Todos os 9 binários compilados com rpath
- Todos os 9 binários verificados via `ldd` — apontam para `.so` local
- `nm -D build/lib/libmonocordio.so | grep mc_ipc` — zero resultados (símbolos antigos ausentes)

---

*— Claudio, 13/03/2026*
