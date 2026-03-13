#include "mc_internal.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MC_MIDI_ENABLED

// ============================================================================
// GLOBAL STATE (Defined in mc_core.c, declared extern in mc_internal.h)
// ============================================================================
// Here we define the FluidSynth specific globals
fluid_settings_t* fluid_settings = NULL;
fluid_synth_t* fluid_synth = NULL;
fluid_player_t* fluid_player = NULL;
bool fluid_enabled = false;

// ============================================================================
// INITIALIZATION / CLEANUP
// ============================================================================

bool MC_Internal_InitFluidSynth(void) {
    fluid_settings = new_fluid_settings();
    if (!fluid_settings) return false;

    // Force sample rate da engine interna
    fluid_settings_setnum(fluid_settings, "synth.sample-rate", MC_SAMPLE_RATE);
    
    // Configurar o player para usar o timer de samples (renderização do synth) ao invés do sistema.
    // Isso evita dessincronização e engasgos quando chamamos o synth no callback da SDL.
    fluid_settings_setstr(fluid_settings, "player.timing-source", "sample");

    fluid_synth = new_fluid_synth(fluid_settings);
    if (!fluid_synth) {
        delete_fluid_settings(fluid_settings);
        return false;
    }

    // Create Player
    fluid_player = new_fluid_player(fluid_synth);
    
    // Tentar carregar SoundFont padrão (Ordem de prioridade)
    const char* sf_paths[] = {
        "/usr/share/sounds/sf2/FluidR3_GM.sf2", // Melhor qualidade
        "/usr/share/sounds/sf2/default-GM.sf2", // Padrão Debian
        "/usr/share/sounds/sf2/TimGM6mb.sf2",   // Leve
        NULL
    };

    bool loaded = false;
    for (int i = 0; sf_paths[i] != NULL; i++) {
        if (fluid_synth_sfload(fluid_synth, sf_paths[i], 1) != FLUID_FAILED) {
            printf("[Monocordio] SoundFont carregado: %s\n", sf_paths[i]);
            loaded = true;
            break;
        }
    }

    if (!loaded) {
         fprintf(stderr, "[Monocordio] ERRO CRÍTICO: Nenhum SoundFont (.sf2) encontrado em /usr/share/sounds/sf2/!\n");
    } else {
        // Gain interno do FluidSynth. 0.2 é o default nativo —
        // valores maiores causam clipping DENTRO do synth em arquivos polifônicos.
        fluid_synth_set_gain(fluid_synth, MC_FLUID_GAIN);
    }
    
    fluid_enabled = true;
    return true;
}

void MC_Internal_CleanupFluidSynth(void) {
    if (fluid_player) delete_fluid_player(fluid_player);
    if (fluid_synth) delete_fluid_synth(fluid_synth);
    if (fluid_settings) delete_fluid_settings(fluid_settings);
    fluid_enabled = false;
}

// ============================================================================
// PUBLIC MIDI API
// ============================================================================

void MC_SendRawMidi(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!fluid_enabled || !fluid_synth) return;

    int type = status & 0xF0;
    int channel = status & 0x0F;

    if (type == 0x90) { // Note On
        if (data2 > 0)
            fluid_synth_noteon(fluid_synth, channel, data1, data2);
        else
            fluid_synth_noteoff(fluid_synth, channel, data1);
    } else if (type == 0x80) { // Note Off
        fluid_synth_noteoff(fluid_synth, channel, data1);
    } else if (type == 0xC0) { // Program Change
        fluid_synth_program_change(fluid_synth, channel, data1);
    } else if (type == 0xB0) { // Control Change
        fluid_synth_cc(fluid_synth, channel, data1, data2);
    }
}

// ============================================================================
// MIDI FILE PLAYER
// ============================================================================

void MC_StopMidiFile(void) {
    if (fluid_player) {
        fluid_player_stop(fluid_player);
        delete_fluid_player(fluid_player);
        fluid_player = NULL; 
    }
}

void MC_PlayMidiFile(const char* filename) {
    if (!fluid_enabled || !fluid_synth) return;

    // Stop previous playback and destroy old player
    MC_StopMidiFile();

    // Create new player
    fluid_player = new_fluid_player(fluid_synth);
    if (!fluid_player) {
         fprintf(stderr, "[Monocordio] Failed to create fluid_player\n");
         return;
    }

    if (fluid_player_add(fluid_player, filename) == FLUID_OK) {
        fluid_player_play(fluid_player);
    } else {
        fprintf(stderr, "[Monocordio] Failed to load MIDI file: %s\n", filename);
        // Clean up
        delete_fluid_player(fluid_player);
        fluid_player = NULL;
    }
}

bool MC_IsMidiPlaying(void) {
    if (!fluid_player) return false;
    return (fluid_player_get_status(fluid_player) == FLUID_PLAYER_PLAYING);
}

void MC_SendSysEx(const uint8_t* data, int length) {
    (void)data; (void)length;
}

#else // MC_MIDI_ENABLED

// Stubs for build without MIDI support
bool MC_Internal_InitFluidSynth(void) { return false; }
void MC_Internal_CleanupFluidSynth(void) {}

void MC_SendRawMidi(uint8_t status, uint8_t data1, uint8_t data2) { (void)status; (void)data1; (void)data2; }
void MC_SendSysEx(const uint8_t* data, int length) { (void)data; (void)length; }

void MC_PlayMidiFile(const char* filename) {
    fprintf(stderr, "[Monocordio] MIDI support disabled in build.\n");
    (void)filename;
}
void MC_StopMidiFile(void) {}
bool MC_IsMidiPlaying(void) { return false; }

#endif // MC_MIDI_ENABLED

// ============================================================================
// MIDI CONVENIENCE API (Sprint 09)
// ============================================================================

static int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void MC_MidiNoteOn(int channel, int note, int velocity) {
    int ch = clamp(channel, 0, 15);
    int n  = clamp(note, 0, 127); 
    int v  = clamp(velocity, 0, 127);
    
    MC_SendRawMidi(0x90 | ch, n, v);
}

void MC_MidiNoteOff(int channel, int note) {
    int ch = clamp(channel, 0, 15);
    int n  = clamp(note, 0, 127);
    
    MC_SendRawMidi(0x80 | ch, n, 0); 
}

void MC_MidiProgramChange(int channel, int patch_number) {
    int ch = clamp(channel, 0, 15);
    int p  = clamp(patch_number, 0, 127);
    
    MC_SendRawMidi(0xC0 | ch, p, 0);
}

void MC_MidiControlChange(int channel, int controller, int value) {
    int ch = clamp(channel, 0, 15);
    int c  = clamp(controller, 0, 127);
    int v  = clamp(value, 0, 127);
    
    MC_SendRawMidi(0xB0 | ch, c, v);
}

void MC_MidiSetVolume(int channel, int volume) {
    MC_MidiControlChange(channel, 7, volume);
}

void MC_MidiSetExpression(int channel, int exp) {
    MC_MidiControlChange(channel, 11, exp);
}

void MC_MidiAllNotesOff(void) {
    for (int i = 0; i < 16; i++) {
        MC_MidiControlChange(i, 123, 0); 
    }
}

void MC_MidiSetPan(int channel, int pan) {
    MC_MidiControlChange(channel, 10, pan); // CC 10 = Pan
}

void MC_MidiSetModulation(int channel, int mod) {
    MC_MidiControlChange(channel, 1, mod); // CC 1 = Modulation Wheel
}

void MC_MidiSetReverb(int channel, int level) {
    MC_MidiControlChange(channel, 91, level); // CC 91 = Reverb Send
}

