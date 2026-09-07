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

    // Disable all channels initially
    rv2a03_write_reg(RV2A03_REG_STATUS, 0x00);

    // Reset frame counter
    rv2a03_write_reg(RV2A03_REG_FRAME_CNT, 0x00);
}

void rv2a03_enable_channels(uint8_t channel_mask) {
    rv2a03_write_reg(RV2A03_REG_STATUS, channel_mask);
}

void rv2a03_mute(void) {
    rv2a03_write_reg(RV2A03_REG_STATUS, 0x00);
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

// Precomputed timer values for standard octave 4 notes (C4..B4)
static const uint16_t note_timer_octave4[12] = {
    238, // C4  (~261.6 Hz)
    225, // C#4 (~277.2 Hz)
    212, // D4  (~293.7 Hz)
    200, // D#4 (~311.1 Hz)
    189, // E4  (~329.6 Hz)
    178, // F4  (~349.2 Hz)
    168, // F#4 (~370.0 Hz)
    159, // G4  (~392.0 Hz)
    150, // G#4 (~415.3 Hz)
    126, // A4  (440.0 Hz, 0x7E)
    119, // A#4 (~466.2 Hz)
    112  // B4  (~493.9 Hz)
};

uint16_t rv2a03_midi_to_pulse_timer(uint8_t midi_note) {
    if (midi_note < 12 || midi_note > 108) return 0x7FF;

    int octave = (midi_note / 12) - 1;
    int note = midi_note % 12;

    uint32_t timer = note_timer_octave4[note];

    if (octave < 4) {
        timer = ((timer + 1) << (4 - octave)) - 1;
    } else if (octave > 4) {
        timer = ((timer + 1) >> (octave - 4)) - 1;
    }

    return (timer > 0x7FF) ? 0x7FF : (uint16_t)timer;
}
