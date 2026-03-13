# 🤝 Handover de Contexto: Polux (Notebook) ➡️ Polux (Desktop)

**Para:** Polux (Instância do Desktop)
**De:** Polux (Instância do Notebook)
**Data:** 13/03/2026

Fala, clone! O Hardy trocou de máquina, então aqui está o download direto na sua memória de tudo que você precisa saber para assumir o projeto **Monocordio** de onde eu parei:

## 1. O Que Acabou de Acontecer
Acabamos de fechar uma rodada intensa de correções guiada por uma revisão apontada pelo *Claudio (Claude Opus 4.6)*, nosso novo Ombudsman e Revisor. 

O Hardy estava sofrendo com um bug de distorção ("rachado") no MIDI e nenhuma mudança de volume que eu fazia no código surtia efeito. O Claudio descobriu que **era um problema de linker dinâmico**: o sistema estava carregando uma velha `libmonocordio.so` da pasta `/usr/local/lib/` em vez da biblioteca recém-compilada da nossa pasta `build/lib/`.

## 2. Soluções Implementadas (Já no Git)
* **Fix de Linkagem:** Adicionei `-Wl,-rpath,'$$ORIGIN/../lib'` no Makefile. Todos os binários agora puxam a `.so` local corretamente.
* **Ganhos e Volumes Balanceados:** A engine 8-bit estava engolindo o MIDI. O padrão agora é `MC_INTERNAL_GAIN 0.8f` e `MC_FLUID_GAIN 0.5f` (configurados no `mc_internal.h`). O exemplo `midi_hybrid.c` soa perfeitamente balanceado agora.
* **Arquitetura Limpa:** 
  - Removi um arquivo fantasma chamado `mc_synth.c` (movi suas funções de utilidade de float para `mc_mml.c`).
  - Removi o campo morto `total_duration` que estava sujando os `mc_presets.c` e os exemplos, mas que a engine já não usava.
  - Implementei um *stub* para a `MC_SendSysEx` no `mc_midi.c` para segurar o contrato do Header.
* **Documentação Redonda:** Os caminhos falsos do `README.md` foram arrumados e a autoria formalizada. O `MANUAL.md` foi atualizado corrigindo matemática, canais máximos documentados e com a adição da documentação do **LFO (Vibrato)**.

## 3. Status Atual
O projeto está compilando sem nenhum *warning* (`make clean && make` roda liso). 
Temos 9 exemplos funcionando em `/examples/`. Toda a parte da arquitetura Híbrida (8 Bits de um lado + FluidSynth MIDI do outro) está pronta e funcional.

## 4. Próximos Passos
O Hardy vai discutir com o *Castor (Arquiteto)* qual é o próximo passo do Backlog. 
Assuma a partir daqui. Esteja pronto para novos épicos!

---
*(Fim da transmissão. Pode deletar este arquivo quando não precisar mais)*
