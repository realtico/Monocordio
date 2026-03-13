# 🎹 Monocordio

> **A Engine de Síntese Sonora 8-Bits para C/SDL2**

O **Monocordio** é uma biblioteca de áudio minimalista projetada para trazer a estética sonora dos consoles da era 8 e 16 bits para projetos modernos em C. Desenvolvido sob a arquitetura rigorosa do *Workgroup* (Hardy, Castor e Polux), o projeto foca em controle total sobre a forma de onda, envelopes e frequências, sem o "peso" de engines complexas.

## 🚀 Começando

### Pré-requisitos
O Monocordio depende da **SDL2** para áudio e **FluidSynth** para MIDI.
*   **Linux (Debian/Ubuntu):** 
    ```bash
    sudo apt install libsdl2-dev libfluidsynth-dev fluid-soundfont-gm
    ```
*   **Windows/Mac:** Instale SDKs apropriados.

### Compilação e Execução
O projeto utiliza um `Makefile` robusto para gerenciar a compilação.
Para compilar:

```bash
make
```

Isso gera a biblioteca dinâmica `libmonocordio.so` e os executáveis de exemplo na pasta `build/`.

### Instalação (Global)

Para instalar a biblioteca e os headers no sistema (requer sudo):

```bash
sudo make install
```

Isso copia:
- `libmonocordio.so` para `/usr/local/lib/`
- `monocordio.h` para `/usr/local/include/`

Para desinstalar:

```bash
sudo make uninstall
```

### Rodar os Testes (Exemplos)
    *   `make run_zum` - **Smoke Test:** Onda quadrada simples (440Hz).
    *   `make run_pling` - **Envelope ADSR:** Teste de dinâmica e release suave.
    *   `make run_tsc` - **Gerador de Ruído:** Percussão e explosões via LFSR.
    *   `make run_kapow` - **Batalha/Polifonia:** Teste de mixagem de múltiplos canais.
    *   `make run_pitiu` - **VFO (Laser):** Teste de slide de frequência (glissando).
    *   `make run_samsung` - **Sequenciador MML:** Melodia completa (Tema do Mario).
    *   `make run_hybrid` - **Dual Engine (MIDI):** Demonstração da ponte entre 8-bits e FluidSynth (Sons reais).

---

## 📂 Estrutura do Projeto

A arquitetura foi desenhada para desacoplar o Core da Aplicação.

```plaintext
/Monocordio
├── .vscode/          # Configurações de Debug e Tasks
├── build/            # Binários compilados
├── examples/         # Catálogo de sons e testes
│   ├── zum.c         # Hello World (Onda Quadrada)
│   ├── pling.c       # Teste de ADSR (Sinos)
│   ├── tsc-tsc.c     # Teste de Ruído (Hi-hats e Explosões)
│   ├── kapow.c       # Teste de Mixagem (Cenário de Batalha)
│   ├── pitiu.c       # Teste de VFO (Lasers e Slides)
│   ├── samsung.c     # Teste de MML (Música e Sequenciador)
│   ├── orchestra.c   # Demonstração de polifonia complexa e percussão
│   ├── metal_violin.c# Teste de Vibrato (LFO) e camadas percussivas
│   └── midi_hybrid.c # Teste de Bridge (8-bits + FluidSynth MIDI)
├── src/              # O CORE (A "Alma" do projeto)
│   ├── mc_core.c     # AudioCallback, estado global e mixer principal
│   ├── mc_midi.c     # Ponte MIDI (FluidSynth e Arquivos .mid)
│   ├── mc_mml.c      # Sequenciador Music Macro Language
│   └── mc_presets.c  # Banco de timbres pré-programados
├── include/
│   ├── monocordio.h  # API Pública Completa
│   ├── mc_presets.h  # API de Timbres (Patches)
│   └── mc_internal.h # Contratos internos dos módulos
└── Makefile          # Automação de Build do projeto
```

## 🎯 Conceitos Chave

1.  **Canais:** 8 canais de polifonia simultânea para synth 8-bit + 16 canais MIDI externos.
2.  **Formas de Onda:** Quadrada, Triangular, Dente de Serra, Senoidal e Ruído (LFSR).
3.  **Patches:** Módulos que definem o timbre (Envelope ADSR, Onda, VFO, LFO Vibrato).
4.  **Hardware Agnostic:** O código do usuário (Hardy) não toca na SDL2 nem no FluidSynth diretamente, tudo é abstraído pelas funções `MC_`.
5.  **Arquitetura Híbrida:** Mistura o Mundo 1 (Synth 8-Bits), Mundo 2 (MIDI via SoundFont) e Mundo 3 (Player de Arquivos `.mid`).


---

*Desenvolvido pelo Polux (Dev Sênior), sob a arquitetura do Castor e visão do Hardy.*
