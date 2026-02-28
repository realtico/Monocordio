#include "mc_internal.h"
#include <stdlib.h>

// ============================================================================
// MML PARSER HELPERS
// ============================================================================

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int parse_number(const char** cursor) {
    int num = 0;
    while (is_digit(**cursor)) {
        num = num * 10 + (**cursor - '0');
        (*cursor)++;
    }
    return num;
}

// ============================================================================
// MML PROCESSOR
// ============================================================================

void MC_Internal_ProcessMML(struct MC_Channel* channel) {
    if (!channel->mml_active || !channel->mml_cursor) return;

    // While we don't have a wait time, process commands
    while (channel->mml_active && channel->mml_wait_samples == 0) {
        char cmd = *channel->mml_cursor;
        
        // End of string 
        if (cmd == '\0') {
            channel->mml_active = false;
            channel->adsr_stage = ADSR_RELEASE; 
            return;
        }

        channel->mml_cursor++; 

        // Skip whitespace
        if (cmd == ' ' || cmd == '\t' || cmd == '\n' || cmd == '\r') continue;

        // Convert to uppercase
        if (cmd >= 'a' && cmd <= 'z') cmd -= 32;
        
        switch (cmd) {
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': 
            case 'R': // Rest
            {
                int note_base = 0;
                int note_idx = -1;
               
                if (cmd == 'A') note_base = 9;
                else if (cmd == 'B') note_base = 11;
                else if (cmd == 'C') note_base = 0;
                else if (cmd == 'D') note_base = 2;
                else if (cmd == 'E') note_base = 4;
                else if (cmd == 'F') note_base = 5;
                else if (cmd == 'G') note_base = 7;
                
                note_idx = note_base;

                // Sharps/Flats
                char next = *channel->mml_cursor;
                if (next == '#' || next == '+') {
                    note_idx++;
                    channel->mml_cursor++;
                } else if (next == '-') {
                    note_idx--;
                    channel->mml_cursor++;
                }

                // Duration
                int length = channel->mml_default_len;
                if (is_digit(*channel->mml_cursor)) {
                    length = parse_number(&channel->mml_cursor);
                }
                
                // Calculate samples
                float samples_per_beat = (MC_SAMPLE_RATE * 60.0f) / (float)channel->mml_tempo;
                float samples_total = (samples_per_beat * 4.0f) / (float)length;
                channel->mml_wait_samples = (uint32_t)samples_total;

                // Trigger (World 1 vs World 2)
                if (cmd != 'R') {
                    if (channel->mode == MC_MODE_INTERNAL) {
                         // World 1: 8-bit Synthesis
                        float freq = MC_Internal_GetNoteFreq(note_idx, channel->mml_octave);
                        channel->frequency = freq;
                        channel->phase = 0.0;
                        channel->stage_timer = 0.0f;
                        channel->vfo_timer = 0.0f;
                        channel->active = true;
                        channel->adsr_stage = ADSR_ATTACK;
                        channel->current_amplitude = 0.0f; 
                    } else {
                        // World 2: FluidSynth MIDI
                        int midi_note = ((channel->mml_octave + 1) * 12) + note_idx;
                        if (midi_note < 0) midi_note = 0; 
                        if (midi_note > 127) midi_note = 127;
                        
                        // Stop previous note
                        if (channel->midi_last_note >= 0) {
                            MC_SendRawMidi(0x80 | (channel - channels), channel->midi_last_note, 0);
                        }
                        
                        // Play new note
                        MC_SendRawMidi(0x90 | (channel - channels), midi_note, 100);
                        channel->midi_last_note = midi_note;
                    }
                    
                } else {
                     // REST
                     if (channel->mode == MC_MODE_INTERNAL) {
                        channel->adsr_stage = ADSR_RELEASE;
                     } else {
                        if (channel->midi_last_note >= 0) {
                            MC_SendRawMidi(0x80 | (channel - channels), channel->midi_last_note, 0);
                            channel->midi_last_note = -1;
                        }
                     }
                }
                break;
            }

            case 'O': // Octave
                if (is_digit(*channel->mml_cursor)) {
                    int oct = parse_number(&channel->mml_cursor);
                    if (oct >= 0 && oct <= 8) channel->mml_octave = oct;
                }
                break;

            case 'T': // Tempo
                 if (is_digit(*channel->mml_cursor)) {
                    channel->mml_tempo = parse_number(&channel->mml_cursor);
                }
                break;

            case 'L': // Length
                if (is_digit(*channel->mml_cursor)) {
                    channel->mml_default_len = parse_number(&channel->mml_cursor);
                }
                break;
                
            case '>': // Octave Up
                if (channel->mml_octave < 8) channel->mml_octave++;
                break;

            case '<': // Octave Down
                if (channel->mml_octave > 0) channel->mml_octave--;
                break;

            case 'V': // Volume (MIDI CC 7)
                if (is_digit(*channel->mml_cursor)) {
                    int vol = parse_number(&channel->mml_cursor);
                    if (channel->mode == MC_MODE_EXTERNAL) {
                        MC_MidiSetVolume((int)(channel - channels), vol);
                    } else {
                        // Map 0-127 to 0.0-1.0
                        channel->volume = (float)vol / 127.0f;
                    }
                }
                break;

            case 'P': // Pan (MIDI CC 10)
                if (is_digit(*channel->mml_cursor)) {
                    int pan = parse_number(&channel->mml_cursor);
                    if (channel->mode == MC_MODE_EXTERNAL) {
                        MC_MidiSetPan((int)(channel - channels), pan);
                    }
                }
                break;
        }
    }
}

void MC_PlayMML(int channel_id, const MC_Patch* patch, const char* mml_string) {
    if (channel_id < 0 || channel_id >= MC_CHANNELS_COUNT) return;

    SDL_LockAudioDevice(audio_device);
    struct MC_Channel* ch = &channels[channel_id];
    
    ch->active = true;
    ch->patch = *patch;
    ch->mml_start = mml_string;
    ch->mml_cursor = mml_string;
    ch->mml_wait_samples = 0;
    ch->mml_octave = 4;
    ch->mml_tempo = 120;
    ch->mml_default_len = 4;
    ch->mml_active = true;
    
    SDL_UnlockAudioDevice(audio_device);
}
