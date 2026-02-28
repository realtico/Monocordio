/**
 * midi_hybrid.c - Sprint 09 Demo
 * Showcases the "Bridge" between 8-bit and MIDI worlds.
 * 
 * Channel 0: Classic 8-bit Square Wave (Mundo 1)
 * Channel 1: FluidSynth Piano (Mundo 2)
 */

#include <stdio.h>
#include <unistd.h>
#include "monocordio.h"
#include "mc_presets.h"

int main() {
    printf("=== Monocordio Sprint 09: The Hybrid Bridge ===\n");
    // Wait a bit for fluidsynth to be ready 
    // (sometimes audio backend takes a moment)
    sleep(1);

    if (!MC_Init()) {
        fprintf(stderr, "Critical Audio Failure.\n");
        return 1;
    }

    printf("Setting up channels...\n");
    // Channel 0: 8-bit Bass (World 1)
    MC_SetChannelMode(0, MC_MODE_INTERNAL); 

    // Channel 1: MIDI Piano (World 2)
    MC_SetChannelMode(1, MC_MODE_EXTERNAL);
    
    // Select Instrument for MIDI Channel 1 (Program Change)
    // 0 = Acoustic Grand Piano (GM Standard)
    MC_MidiProgramChange(1, 0); // Piano

    // Set Max Volume (CC 7) and Expression (CC 11) for Channel 1
    MC_MidiSetVolume(1, 127);
    MC_MidiSetExpression(1, 127);

    const char* mml_bass = "T100 O3 C4 R4 G4 R4 A4 R4 F4 R4"; // Simple walking bass
    const char* mml_piano = "T100 O5 E8 G8 C8 E8 D8 F8 A8 G8 F4 E4 D4 C4"; // Arpeggio

    printf("Playing Duet: 8-bit Bass vs MIDI Piano...\n");
    
    // Patches are irrelevant for MIDI mode, but required by API signature.
    // Except for internal mode channel 0.
    MC_PlayMML(0, &MC_PATCH_ORGAN, mml_bass); // Using standard organ (square)
    MC_PlayMML(1, &MC_PATCH_PIANO, mml_piano); // Patch ignored by MIDI mode logic, but passed safely

    // Wait for the loop
    MC_Wait(4000);

    /* Test Raw MIDI Commands (Mundo 3) - Now via Friendly API */
    printf("\nTesting Raw MIDI Injection (Mundo 3)...\n");
    printf("Sending direct Chord to Channel 1...\n");
    
    // Major Chord C4, E4, G4 on Channel 1
    MC_MidiNoteOn(1, 60, 100);
    MC_MidiNoteOn(1, 64, 100);
    MC_MidiNoteOn(1, 67, 100);
    
    MC_Wait(1000);
    
    MC_MidiNoteOff(1, 60);
    MC_MidiNoteOff(1, 64);
    MC_MidiNoteOff(1, 67);

    printf("Done.\n");
    MC_MidiAllNotesOff(); // Safety Shutdown

    /* Mundo 3b: The Forked Player */
    printf("\n=== Testing IPC Midi Player (Sprint 09) ===\n");
    // Using the asset provided by Hardy
    MC_PlayMidiFile("assets/Never-Gonna-Give-You-Up-3.mid");
    
    // Let it play for 10 seconds (parent process runs this wait)
    printf("Playing background MIDI file via pipe...\n");
    for(int i=0; i<10; i++) {
        printf("."); 
        fflush(stdout);
        MC_Wait(1000);
    }
    printf("\nStopping Player.\n");
    
    MC_Cleanup(); // This kills the player child
    return 0;
}
