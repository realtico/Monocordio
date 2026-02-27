# Parecer do Claudio — Ombudsman do Código

## Impressão Geral

O Monocordio é um projeto surpreendentemente coerente para algo saído de duas instâncias de IA como "ghostwriters". A arquitetura é limpa, a API é minimalista (`MC_Init`, `MC_Play`, `MC_Stop`, `MC_PlayMML`, `MC_Cleanup`, `MC_Wait` — 6 funções, cabe num post-it), e os exemplos formam uma progressão didática excelente: zum (smoke test) -> pling (ADSR) -> tsc (ruído) -> pitiu (VFO) -> kapow (polifonia) -> samsung (MML) -> orchestra (tudo junto). Isso é raro até em projetos humanos.

Dito isso, há coisas que precisam ser ditas.

---

## 1. CLAREZA

### O que está bom:
- A nomenclatura da API é impecável. `MC_Play`, `MC_Stop`, `MC_PlayMML` — qualquer programador C entende em 2 segundos.
- Os presets em `include/mc_presets.h` com comentários descritivos ("Sine + Soft Envelope", "Noise + Snap") são um toque profissional.
- A documentação (README + MANUAL) é acima da média para uma lib C pessoal.

### O que precisa melhorar:

- **O header expõe `MC_Channel` inteiro ao usuário.** Isso é um vazamento de implementação. O usuário nunca deveria ver `lfsr_reg`, `mml_cursor`, `adsr_stage`. Esse struct deveria estar no `.c` (opaco), e o header deveria ter apenas os typedefs públicos (`MC_Patch`, `MC_Waveform`, `MC_ADSR`, `MC_VFO`) e as assinaturas de função. Isso é a regra de ouro em C: *"o header é o contrato, o .c é o segredo"*.

- **README diz `libs/monocordio/` mas a estrutura real é `src/` + `include/`.** O README mente. Isso é o tipo de dessincronia clássica de projetos com IA — uma gerou a doc, outra reorganizou os diretórios. Corrija isso, pois é a primeira coisa que alguém que clonar o repo vai notar.

- **A pasta `patches/` existe e está vazia.** Remove ou usa. Diretório fantasma é ruído cognitivo.

- **Comentários internos em inglês, docs em português.** Escolha um idioma e seja consistente, ou pelo menos documente a regra ("API em inglês, docs em PT-BR"). Misturar `printf("Iniciando Protocolo Monocordio...")` com `// ADSR Stages` cria dissonância.

---

## 2. ESTABILIDADE

### Problemas reais encontrados:

- **Race condition no `MC_Play` + MML.** Se alguém chama `MC_PlayMML` num canal e depois `MC_Play` no mesmo canal, o `mml_active` não é resetado pelo `MC_Play`. O sequenciador MML vai continuar tentando processar no próximo callback. `MC_Play` em `src/monocordio.c` (função MC_Play, ~linha 270) deveria incluir `channels[channel_id].mml_active = false;`.

- **O LFSR reseta com seed fixo (`0xACE1`) em cada `MC_Play`.** Isso significa que *toda* explosão e *todo* hi-hat começam com exatamente a mesma sequência de ruído. Num uso rápido (burst de hi-hats), o ouvido percebe a repetição — soa "mecânico". Use um seed derivado de algo variável (e.g. `SDL_GetTicks()` ou um contador global).

- **Release rate assume decay de 1.0.** Como o próprio comentário no código confessa: o release decrementa com slope `1.0 / release_time`, mas se o nível atual for 0.3 (sustain), o release demora 3x mais que o necessário para chegar a zero. Guarde `release_start_level` no canal quando entrar em RELEASE e use `envelope -= (release_start_level / release_time) * dt`.

- **`MC_Play(0, &MC_PATCH_ORGAN, 0)` no orchestra.c — freq=0.** Isso gera um `phase_increment` de 0, o canal fica ativo com amplitude crescendo (attack), gerando um DC offset mudo que ocupa o canal à toa. É um bug no exemplo, mas a lib deveria tratar `freq <= 0` como no-op ou clampear no mínimo.

- **`total_duration` e MML interagem mal.** Se um patch tem `total_duration = 0` (como nos patches do samsung.c), o timer nunca força release. Tudo bem. Mas se alguém usar um patch com `total_duration > 0` no MML, o timer vai começar a cortar notas independente do ritmo do MML. Documente essa interação ou force `total_duration = 0` quando MML está ativo.

- **Buffer de áudio é mono, 1024 samples.** Para 44100Hz, isso dá ~23ms de latência. Está ok para uma lib retro, mas vale documentar. Em engines de jogo, 512 samples é mais comum para input responsivo.

---

## 3. UX (Experiência do Desenvolvedor)

### O que impressiona:
- Os nomes dos exemplos (zum, pling, tsc-tsc, kapow, pitiu, samsung) são onomatopeias geniais. Qualquer dev vai lembrar o que cada um faz.
- O `MC_PlayExplosion()` como helper de alto nível mostrando que 2 canais fazem um efeito composto é um padrão excelente. Façam mais disso.
- O Makefile com `run_*` targets usando `LD_LIBRARY_PATH` é conveniente.

### O que atrapalha:

- **Não há `MC_IsPlaying(int channel)`.** O usuário não tem como saber quando um som acabou, exceto por adivinhação temporal com `MC_Wait`. Para qualquer uso real (jogo, app), isso é obrigatório.

- **`MC_Wait` é um `SDL_Delay`, bloqueia a thread.** Funciona para demos sequenciais, mas num game loop real é veneno. O MANUAL deveria alertar explicitamente: "*`MC_Wait` é para exemplos e testes. Em produção, use o game loop para controlar timing.*"

- **Não há controle de volume por canal.** A mixagem atual é simplesmente soma linear. 8 canais tocando ao mesmo tempo com amplitude 1.0 = clipping garantido (soma > 1.0, cortada por hard clip). A assinatura deveria ser `MC_Play(channel, patch, freq, volume)` ou haver um `MC_SetVolume(channel, float vol)`.

- **Falta `MC_SetMasterVolume(float vol)`.** O `master_volume` é hardcoded em 0.3f. É um valor razoável, mas deveria ser acessível pela API.

---

## 4. OS SONS CHIADOS (Bateria e Explosão)

Este é o ponto técnico mais importante. O "chiado" que vocês ouvem tem **três causas raiz**:

### Causa 1: O LFSR roda na taxa de amostragem (44100Hz)

O `generate_noise()` é chamado **uma vez por sample**. Isso gera ruído branco puro, que tem muita energia em altas frequências — *é* chiado por definição. Nos chips reais (NES, Game Boy, SN76489), o shift register roda a uma **frequência sub-dividida** do clock. A frequência passada para `MC_Play` deveria controlar a *"clock rate"* do LFSR, não ser ignorada.

**Sugestão concreta:** Use o parâmetro `freq` para controlar a taxa de atualização do LFSR. O `phase` accumulator já existe — use ele:

```c
case MC_WAVE_NOISE_LFSR:
    // Only clock the LFSR when phase wraps around
    if (channel->phase >= 2.0 * M_PI) {
        channel->phase -= 2.0 * M_PI;
        // Clock the shift register
        sample_val = generate_noise(&channel->lfsr_reg);
        channel->last_noise_sample = sample_val; // store it
    } else {
        sample_val = channel->last_noise_sample; // hold previous value
    }
    break;
```

O efeito: com `freq=1000`, o ruído muda 1000x por segundo (metálico, hi-hat). Com `freq=200`, muda 200x (mais "grave", explosão grossa). Com `freq=50`, é um rugido grave de motor. **Isso é exatamente como o NES e o Game Boy fazem.** E de repente, a frequência passa a importar para ruído, e a UX fica mais rica.

### Causa 2: Falta de filtragem

Ruído branco puro é cientificamente desagradável. Um filtro passa-baixa simples (1-pole IIR) suaviza drasticamente:

```c
// After generating raw noise sample:
float alpha = 0.3f; // 0.0=sem filtro, 1.0=mudo. Ajustar a gosto.
sample_val = alpha * sample_val + (1.0f - alpha) * channel->last_noise_sample;
channel->last_noise_sample = sample_val;
```

Para explosões, `alpha ≈ 0.6-0.7` (mais grave). Para hi-hat, `alpha ≈ 0.1-0.2` (mais brilhante). Isso poderia ser um campo no `MC_Patch` (e.g. `float noise_filter`).

### Causa 3: Hard clipping na mixagem

Quando vários canais de ruído + ondas somam tudo, a hard clip no final (`if > 1.0, = 1.0`) gera distorção digital, que soa como... chiado adicional. Considere soft clipping:

```c
// Em vez de hard clamp:
sample_mix = tanhf(sample_mix); // Soft saturation
```

Ou simplesmente divida pela contagem de canais ativos.

---

## 5. PITACOS DE SABEDORIA

1. **Versionem a API.** Adicionem `MC_VERSION_MAJOR`, `MC_VERSION_MINOR` no header. Quando mudarem a assinatura de `MC_Play`, quem depender da lib vai agradecer.

2. **O `MC_Patch` deveria ser `const`-friendly.** Já é, mas os presets são `extern const` no header e `const` no .c — isso é correto e bom. Mantenham.

3. **A callback de áudio faz *tudo*.** MML parsing, ADSR, VFO, waveform generation, mixing — tudo numa função de ~120 linhas chamada em contexto de áudio. Funciona com 8 canais, mas se um dia quiserem mais, separem o mixing em subfunções (`process_adsr()`, `process_vfo()`, `generate_sample()`). Não é urgente, mas é higiênico.

4. **Testem com Valgrind.** A lib não tem `malloc`/`free` (ótimo!), mas a SDL sim. Um `valgrind --leak-check=full` nos exemplos dá tranquilidade.

5. **Considerem `-O2` no CFLAGS para a .so** e `-g` só nos exemplos. `sinf()` e `powf()` são chamados por sample — otimização do compilador faz diferença audível na latência.

6. **O campo `total_duration` no `MC_Patch` é uma idiossincrasia.** Ele mistura *patch* (timbre) com *nota* (duração). Nos sintetizadores reais, o patch define *como* soa, e quem toca define *quanto tempo*. Considere mover isso para um parâmetro de `MC_Play` ou depreciar em favor do controle externo via `MC_Stop`.

---

## Veredicto

O Monocordio *faz o que promete*: é simples, produz som, e um dev C mediano consegue usar em 5 minutos. A arquitetura é sólida para o escopo proposto. Os problemas que encontrei são de **polimento**, não de fundação — o que é um elogio considerando que duas IAs escreveram isso.

### As 3 mudanças de maior impacto seriam:
1. **LFSR com clock controlado por frequência** (mata o chiado E dá expressividade)
2. **Esconder `MC_Channel` do header** (limpeza de API)
3. **`MC_IsPlaying()` + volume por canal** (usabilidade real)

**Nota do Claudio: 7.5/10** — tem alma de produto, mas precisa de um polimento para virar algo que alguém publicaria no GitHub com orgulho.
