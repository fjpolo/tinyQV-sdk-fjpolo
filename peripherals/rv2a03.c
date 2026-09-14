/*
 * SPDX-License-Identifier: Apache-2.0
 * RV2A03 NES APU Sound Peripheral Driver Implementation for TinyQV
 * Target: Tiny Tapeout Sky25a (Berzerk instance)
 */

#include "rv2a03.h"

// Reference timer period for Square channel at A4 (440Hz) is 126 (0x7E)
// Effective APU pulse timer clock base: ~894,080 Hz
#define APU_PULSE_CLOCK_BASE    894080UL
#define APU_TRI_CLOCK_BASE      894080UL

void rv2a03_init(void) {
    // Enable clock in peripheral Configuration 0
    rv2a03_write_reg(RV2A03_REG_CONFIG0, RV2A03_CFG_CE);

    // Mute all channels cleanly to prevent DC leakage on taped-out silicon
    rv2a03_mute();

    // Reset frame counter
    rv2a03_write_reg(RV2A03_REG_FRAME_CNT, 0x00);
}

void rv2a03_enable_channels(uint8_t channel_mask) {
    // Hardware adaptation for taped-out APU silicon:
    // Register $4015 write decode is sampled on apu_ce_sync (8 out of 12 clock cycles).
    // Writing across a 16-cycle burst guarantees the write lands in the active window.
    for (int i = 0; i < 16; i++) {
        rv2a03_write_reg(RV2A03_REG_STATUS, channel_mask);
    }
}

void rv2a03_mute(void) {
    // Hardware Workaround for Taped-Out Silicon:
    // 1. Force Pulse 1 & Pulse 2 volume to 0 (constant volume 1, vol 0)
    rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
    rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);

    // 2. Clear Triangle linear counter
    rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);

    // 3. Force Noise volume to 0 (constant volume 1, vol 0)
    rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);

    // 4. Disable all channels in status register ($4015)
    for (int i = 0; i < 16; i++) {
        rv2a03_write_reg(RV2A03_REG_STATUS, 0x00);
    }
}

void rv2a03_set_pulse1(uint8_t duty, uint8_t volume, bool const_vol, bool loop_env,
                       uint16_t timer, uint8_t length_idx) {
    uint8_t reg0 = (duty & 0xC0) |
                   (loop_env ? RV2A03_ENV_LOOP : 0) |
                   (const_vol ? RV2A03_ENV_CONST_VOL : 0) |
                   (volume & 0x0F);

    rv2a03_write_reg(RV2A03_REG_SQ1_VOL, reg0);
    rv2a03_write_reg(RV2A03_REG_SQ1_SWEEP, 0x00); // Sweep off by default
    rv2a03_write_reg(RV2A03_REG_SQ1_LO, (uint8_t)(timer & 0xFF));
    rv2a03_write_reg(RV2A03_REG_SQ1_HI, (uint8_t)(((length_idx & 0x1F) << 3) | ((timer >> 8) & 0x07)));
}

void rv2a03_set_pulse2(uint8_t duty, uint8_t volume, bool const_vol, bool loop_env,
                       uint16_t timer, uint8_t length_idx) {
    uint8_t reg0 = (duty & 0xC0) |
                   (loop_env ? RV2A03_ENV_LOOP : 0) |
                   (const_vol ? RV2A03_ENV_CONST_VOL : 0) |
                   (volume & 0x0F);

    rv2a03_write_reg(RV2A03_REG_SQ2_VOL, reg0);
    rv2a03_write_reg(RV2A03_REG_SQ2_SWEEP, 0x00);
    rv2a03_write_reg(RV2A03_REG_SQ2_LO, (uint8_t)(timer & 0xFF));
    rv2a03_write_reg(RV2A03_REG_SQ2_HI, (uint8_t)(((length_idx & 0x1F) << 3) | ((timer >> 8) & 0x07)));
}

void rv2a03_set_triangle(uint8_t linear_reload, bool halt, uint16_t timer, uint8_t length_idx) {
    uint8_t reg0 = (halt ? RV2A03_TRI_CONTROL_HALT : 0) | (linear_reload & 0x7F);

    rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, reg0);
    rv2a03_write_reg(RV2A03_REG_TRI_LO, (uint8_t)(timer & 0xFF));
    rv2a03_write_reg(RV2A03_REG_TRI_HI, (uint8_t)(((length_idx & 0x1F) << 3) | ((timer >> 8) & 0x07)));
}

void rv2a03_set_noise(uint8_t volume, bool const_vol, bool loop_env,
                      uint8_t period_idx, bool short_mode, uint8_t length_idx) {
    uint8_t reg0 = (loop_env ? RV2A03_ENV_LOOP : 0) |
                   (const_vol ? RV2A03_ENV_CONST_VOL : 0) |
                   (volume & 0x0F);

    uint8_t reg1 = (short_mode ? RV2A03_NOISE_MODE_93 : 0) | (period_idx & 0x0F);

    rv2a03_write_reg(RV2A03_REG_NOISE_VOL, reg0);
    rv2a03_write_reg(RV2A03_REG_NOISE_LO, reg1);
    rv2a03_write_reg(RV2A03_REG_NOISE_HI, (uint8_t)((length_idx & 0x1F) << 3));
}

uint16_t rv2a03_calc_pulse_timer(uint32_t freq_hz) {
    if (freq_hz == 0) return 0x7FF;
    uint32_t val = (APU_PULSE_CLOCK_BASE / (16 * freq_hz));
    if (val == 0) return 0;
    val -= 1;
    return (val > 0x7FF) ? 0x7FF : (uint16_t)val;
}

uint16_t rv2a03_calc_tri_timer(uint32_t freq_hz) {
    if (freq_hz == 0) return 0x7FF;
    uint32_t val = (APU_TRI_CLOCK_BASE / (32 * freq_hz));
    if (val == 0) return 0;
    val -= 1;
    return (val > 0x7FF) ? 0x7FF : (uint16_t)val;
}

// Precomputed timer values for all octave notes (Octaves 0 to 8)
// Tuned for A4 (440 Hz) = 126 (0x7E) with base clock 894,080 Hz
static const uint16_t note_timer_octave0[12] = {
    2047, // C0   (~  16.4 Hz)
    2047, // C#0  (~  17.3 Hz)
    2047, // D0   (~  18.4 Hz)
    2047, // D#0  (~  19.4 Hz)
    2047, // E0   (~  20.6 Hz)
    2047, // F0   (~  21.8 Hz)
    2047, // F#0  (~  23.1 Hz)
    2047, // G0   (~  24.5 Hz)
    2047, // G#0  (~  26.0 Hz)
    2031, // A0   (~  27.5 Hz)
    1917, // A#0  (~  29.1 Hz)
    1809  // B0   (~  30.9 Hz)
};

static const uint16_t note_timer_octave1[12] = {
    1708, // C1   (~  32.7 Hz)
    1612, // C#1  (~  34.6 Hz)
    1521, // D1   (~  36.7 Hz)
    1436, // D#1  (~  38.9 Hz)
    1355, // E1   (~  41.2 Hz)
    1279, // F1   (~  43.7 Hz)
    1207, // F#1  (~  46.2 Hz)
    1139, // G1   (~  49.0 Hz)
    1075, // G#1  (~  51.9 Hz)
    1015, // A1   (~  55.0 Hz)
     958, // A#1  (~  58.3 Hz)
     904  // B1   (~  61.7 Hz)
};

static const uint16_t note_timer_octave2[12] = {
     853, // C2   (~  65.4 Hz)
     805, // C#2  (~  69.3 Hz)
     760, // D2   (~  73.4 Hz)
     717, // D#2  (~  77.8 Hz)
     677, // E2   (~  82.4 Hz)
     639, // F2   (~  87.3 Hz)
     603, // F#2  (~  92.5 Hz)
     569, // G2   (~  98.0 Hz)
     537, // G#2  (~ 103.8 Hz)
     507, // A2   (~ 110.0 Hz)
     478, // A#2  (~ 116.5 Hz)
     452  // B2   (~ 123.5 Hz)
};

static const uint16_t note_timer_octave3[12] = {
     426, // C3   (~ 130.8 Hz)
     402, // C#3  (~ 138.6 Hz)
     380, // D3   (~ 146.8 Hz)
     358, // D#3  (~ 155.6 Hz)
     338, // E3   (~ 164.8 Hz)
     319, // F3   (~ 174.6 Hz)
     301, // F#3  (~ 185.0 Hz)
     284, // G3   (~ 196.0 Hz)
     268, // G#3  (~ 207.7 Hz)
     253, // A3   (~ 220.0 Hz)
     239, // A#3  (~ 233.1 Hz)
     225  // B3   (~ 246.9 Hz)
};

static const uint16_t note_timer_octave4[12] = {
     213, // C4   (~ 261.6 Hz)
     201, // C#4  (~ 277.2 Hz)
     189, // D4   (~ 293.7 Hz)
     179, // D#4  (~ 311.1 Hz)
     169, // E4   (~ 329.6 Hz)
     159, // F4   (~ 349.2 Hz)
     150, // F#4  (~ 370.0 Hz)
     142, // G4   (~ 392.0 Hz)
     134, // G#4  (~ 415.3 Hz)
     126, // A4   (440.0 Hz, 0x7E)
     119, // A#4  (~ 466.2 Hz)
     112  // B4   (~ 493.9 Hz)
};

static const uint16_t note_timer_octave5[12] = {
     106, // C5   (~ 523.3 Hz)
     100, // C#5  (~ 554.4 Hz)
      94, // D5   (~ 587.3 Hz)
      89, // D#5  (~ 622.3 Hz)
      84, // E5   (~ 659.3 Hz)
      79, // F5   (~ 698.5 Hz)
      75, // F#5  (~ 740.0 Hz)
      70, // G5   (~ 784.0 Hz)
      66, // G#5  (~ 830.6 Hz)
      63, // A5   (~ 880.0 Hz)
      59, // A#5  (~ 932.3 Hz)
      56  // B5   (~ 987.8 Hz)
};

static const uint16_t note_timer_octave6[12] = {
      52, // C6   (~1046.5 Hz)
      49, // C#6  (~1108.7 Hz)
      47, // D6   (~1174.7 Hz)
      44, // D#6  (~1244.5 Hz)
      41, // E6   (~1318.5 Hz)
      39, // F6   (~1396.9 Hz)
      37, // F#6  (~1480.0 Hz)
      35, // G6   (~1568.0 Hz)
      33, // G#6  (~1661.2 Hz)
      31, // A6   (~1760.0 Hz)
      29, // A#6  (~1864.7 Hz)
      27  // B6   (~1975.5 Hz)
};

static const uint16_t note_timer_octave7[12] = {
      26, // C7   (~2093.0 Hz)
      24, // C#7  (~2217.5 Hz)
      23, // D7   (~2349.3 Hz)
      21, // D#7  (~2489.0 Hz)
      20, // E7   (~2637.0 Hz)
      19, // F7   (~2793.8 Hz)
      18, // F#7  (~2960.0 Hz)
      17, // G7   (~3136.0 Hz)
      16, // G#7  (~3322.4 Hz)
      15, // A7   (~3520.0 Hz)
      14, // A#7  (~3729.3 Hz)
      13  // B7   (~3951.1 Hz)
};

static const uint16_t note_timer_octave8[12] = {
      12, // C8   (~4186.0 Hz)
      12, // C#8  (~4434.9 Hz)
      11, // D8   (~4698.6 Hz)
      10, // D#8  (~4978.0 Hz)
      10, // E8   (~5274.0 Hz)
       9, // F8   (~5587.7 Hz)
       8, // F#8  (~5919.9 Hz)
       8, // G8   (~6271.9 Hz)
       7, // G#8  (~6644.9 Hz)
       7, // A8   (~7040.0 Hz)
       6, // A#8  (~7458.6 Hz)
       6  // B8   (~7902.1 Hz)
};

// Pointer array indexing all 9 octaves (0 to 8)
static const uint16_t* const note_timer_octaves[9] = {
    note_timer_octave0,
    note_timer_octave1,
    note_timer_octave2,
    note_timer_octave3,
    note_timer_octave4,
    note_timer_octave5,
    note_timer_octave6,
    note_timer_octave7,
    note_timer_octave8
};

uint16_t rv2a03_midi_to_pulse_timer(uint8_t midi_note) {
    if (midi_note < 12 || midi_note > 119) return 0x7FF;

    int octave = (midi_note / 12) - 1;
    int note = midi_note % 12;

    if (octave >= 0 && octave <= 8) {
        return note_timer_octaves[octave][note];
    }

    return 0x7FF;
}

