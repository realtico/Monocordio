# 🎹 Monocordio

> **A Engine de Síntese Sonora 8-Bits para C/SDL2**

O **Monocordio** é uma biblioteca de áudio minimalista projetada para trazer a estética sonora dos consoles da era 8 e 16 bits para projetos modernos em C. Desenvolvido sob a arquitetura rigorosa do *Workgroup* (Hardy, Castor e Polux), o projeto foca em controle total sobre a forma de onda, envelopes e frequências, sem o "peso" de engines complexas.

## 🚀 Começando

### Pré-requisitos
O Monocordio depende apenas da **SDL2** para o output de áudio.
*   **Linux (Debian/Ubuntu):** `sudo apt install libsdl2-dev`
*   **Windows/Mac:** Instale o SDK do SDL2 e ajuste o Makefile se necessário.

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
│   └── samsung.c     # Teste de MML (Música e Sequenciador)
├── libs/
│   └── monocordio/   # O CORE (A "Alma" do projeto)
│       ├── monocordio.h  # Contrato de Dados (API Pública)
│       └── monocordio.c  # Implementação da Síntese e Mixer
└── Makefile          # Automação de Build
```

## 🎯 Conceitos Chave

1.  **Canais:** 8 canais de polifonia simultânea.
2.  **Formas de Onda:** Quadrada, Triangular, Dente de Serra, Senoidal e Ruído (LFSR 16-bit).
3.  **Patches:** Estruturas de dados que definem o timbre (Envelope ADSR, Onda, VFO).
4.  **Hardware Agnostic:** O código do usuário (Hardy) não toca na SDL2 diretamente, usando as funções wrapper `MC_`.

---

*Desenvolvido pelo Polux (Dev Sênior), sob a arquitetura do Castor e visão do Hardy.*
