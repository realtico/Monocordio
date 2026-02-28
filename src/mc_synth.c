#include "mc_internal.h"
#include <math.h>

// ============================================================================
// SYNTHESIS HELPERS
// ============================================================================

float MC_Internal_GetNoteFreq(int note_index, int octave) {
    // MIDI note calculation
    int midi_note = (octave + 1) * 12 + note_index;
    return 440.0f * powf(2.0f, (midi_note - 69.0f) / 12.0f);
}

// NOTE: The actual waveform generation loop is currently tightly coupled into 
// AudioCallback in mc_core.c to maximize performance and minimize function call overhead 
// inside the sample loop. 
// If we wanted to fully abstract it, we would create a `MC_GenerateSample(channel)`
// function here. For now, the prompt asked to separate responsibilities. 
// The AudioCallback logic is in Core.
// However, `MC_Internal_GetNoteFreq` properly belongs here.

