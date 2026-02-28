#include "mc_internal.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// GLOBAL STATE (Defined in mc_core.c, declared extern in mc_internal.h)
// ============================================================================
// Here we define the FluidSynth specific globals
fluid_settings_t* fluid_settings = NULL;
fluid_synth_t* fluid_synth = NULL;
bool fluid_enabled = false;

// ============================================================================
// INITIALIZATION / CLEANUP
// ============================================================================

bool MC_Internal_InitFluidSynth(void) {
    fluid_settings = new_fluid_settings();
    if (!fluid_settings) return false;

    // Force sample rate da engine interna
    fluid_settings_setnum(fluid_settings, "synth.sample-rate", MC_SAMPLE_RATE);

    fluid_synth = new_fluid_synth(fluid_settings);
    if (!fluid_synth) {
        delete_fluid_settings(fluid_settings);
        return false;
    }

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
        // Aumentar ganho para audibilidade clara (2.0f é um bom boost para competir com square waves)
        fluid_synth_set_gain(fluid_synth, 2.0f);
    }
    
    fluid_enabled = true;
    return true;
}

void MC_Internal_CleanupFluidSynth(void) {
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

// ============================================================================
// MIDI FILE PLAYER (Mundo 3b)
// ============================================================================

void MC_PlayMidiFile(const char* filename) {
    if (mc_ipc_active) {
        // Kill previous player
        kill(mc_player_pid, SIGTERM);
        waitpid(mc_player_pid, NULL, 0);
        close(mc_pipe_fd[0]);
        mc_ipc_active = false;
    }

    if (pipe(mc_pipe_fd) == -1) {
        perror("pipe");
        return;
    }

    // Set read end to non-blocking (CRITICAL for AudioCallback)
    int flags = fcntl(mc_pipe_fd[0], F_GETFL, 0);
    fcntl(mc_pipe_fd[0], F_SETFL, flags | O_NONBLOCK);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // CHILD PROCESS (The Player)
        close(mc_pipe_fd[0]); // Close read end of pipe

        // Open MIDI file
        FILE* f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Child: Failed to open %s\n", filename);
            _exit(1);
        }

        // --- Inline Macros (using _exit on error since we are in child) ---
        uint8_t b4[4], b2[2], b1;
        #define CHECK_READ(count) if(fread(b4,1,count,f)!=count) { fclose(f); _exit(1); }
        #define READ_U32(val) { if(fread(b4,1,4,f)!=4) { fclose(f); _exit(1); } val = (b4[0]<<24)|(b4[1]<<16)|(b4[2]<<8)|b4[3]; }
        #define READ_U16(val) { if(fread(b2,1,2,f)!=2) { fclose(f); _exit(1); } val = (b2[0]<<8)|b2[1]; }
        #define READ_VAR(val) { val=0; while(1){ if(fread(&b1,1,1,f)!=1) { fclose(f); _exit(1); } val=(val<<7)|(b1&0x7F); if(!(b1&0x80)) break; } }

        uint32_t chunk_type, chunk_len;
        READ_U32(chunk_type);
        READ_U32(chunk_len);

        if (chunk_type != 0x4D546864) { // "MThd"
             fprintf(stderr, "Child: Not a MIDI file\n");
             fclose(f); _exit(1);
        }
        
        uint16_t fmt, trks, div;
        READ_U16(fmt); READ_U16(trks); READ_U16(div);
        
        int tracks_found = 0;
        
        while (tracks_found < trks) {
             READ_U32(chunk_type);
             READ_U32(chunk_len);
             
             if (chunk_type == 0x4D54726B) { // "MTrk"
                 long start_pos = ftell(f);
                 long end_pos = start_pos + chunk_len;
                 
                 uint8_t running_status = 0;
                 uint32_t us_per_q = 500000; // 120 BPM default

                 while (ftell(f) < end_pos) {
                     uint32_t delta;
                     READ_VAR(delta);
                     
                     if (delta > 0) {
                         // Simple overflow protection
                         uint64_t wait = (uint64_t)delta * us_per_q / div;
                         if (wait > 2000000) wait = 2000000; 
                         usleep(wait);
                     }
                     
                     if (fread(&b1, 1, 1, f) != 1) break;
                     
                     if (b1 & 0x80) running_status = b1;
                     else { fseek(f, -1, SEEK_CUR); b1 = running_status; }
                     
                     if (b1 == 0xFF) { // Meta
                         uint8_t type; fread(&type, 1, 1, f);
                         uint32_t len; READ_VAR(len);
                         if (type == 0x51 && len == 3) {
                              fread(b4, 1, 3, f); // Recycle b4
                              us_per_q = (b4[0]<<16)|(b4[1]<<8)|b4[2];
                         } else {
                              fseek(f, len, SEEK_CUR);
                         }
                     } else if ((b1 & 0xF0) == 0xF0) { // Sysex
                         uint32_t len; READ_VAR(len);
                         fseek(f, len, SEEK_CUR);
                     } else { // Channel Msg
                         uint8_t cmd = b1 & 0xF0;
                         uint8_t p1=0, p2=0;
                         fread(&p1, 1, 1, f); 
                         if (cmd != 0xC0 && cmd != 0xD0) fread(&p2, 1, 1, f);
                         
                         uint8_t msg[] = {b1, p1, p2};
                         int to_write = (cmd==0xC0 || cmd==0xD0) ? 2 : 3;
                         write(mc_pipe_fd[1], msg, to_write);
                     }
                 }
                 break; 
             } else {
                 fseek(f, chunk_len, SEEK_CUR);
             }
             tracks_found++;
        }
        
        fclose(f);
        close(mc_pipe_fd[1]);
        _exit(0);
        
    } else {
        // PARENT
        close(mc_pipe_fd[1]); // Close write end
        mc_ipc_active = true;
        mc_player_pid = pid;
    }
}
