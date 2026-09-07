/*
 * SPDX-License-Identifier: Apache-2.0
 * RV2A03 NES APU Sound Peripheral Driver for TinyQV
 * Target: Tiny Tapeout Sky25a (Berzerk instance)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "rv2a03_regs.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// Basic Hardware Access Primitives
// --------------------------------------------------------------------------

static inline void rv2a03_write_reg(uint8_t reg, uint8_t val) {
    RV2A03_BASE[reg] = val;
}

static inline uint8_t rv2a03_read_reg(uint8_t reg) {
    return RV2A03_BASE[reg];
}

/**
 * Reads the current 16-bit mixed audio output sample from registers 0x24/0x25.
 * Returns signed 16-bit PCM value.
 */
static inline int16_t rv2a03_read_sample(void) {
    uint8_t msb = RV2A03_BASE[RV2A03_REG_OUTPUT_MSB];
    uint8_t lsb = RV2A03_BASE[RV2A03_REG_OUTPUT_LSB];
    return (int16_t)((msb << 8) | lsb);
}

// --------------------------------------------------------------------------
// Initialization & Channel Control
// --------------------------------------------------------------------------

/**
 * Initializes the RV2A03 peripheral:
 * - Enables peripheral clock (CE bit in 0x20)
 * - Mutes and disables all audio channels (0x15 = 0x00)
 * - Resets frame counter (0x17 = 0x00)
 */
void rv2a03_init(void);

/**
 * Enables or disables individual APU channels via register 0x15.
 * @param channel_mask Bitwise OR of RV2A03_STATUS_*_ENABLE
 */
void rv2a03_enable_channels(uint8_t channel_mask);

/**
 * Mutes all sound channels immediately.
 */
void rv2a03_mute(void);

// --------------------------------------------------------------------------
// Channel Configuration API
// --------------------------------------------------------------------------

/**
 * Configures Pulse (Square) Channel 1.
 * @param duty       RV2A03_DUTY_12_5, RV2A03_DUTY_25, RV2A03_DUTY_50, RV2A03_DUTY_75
 * @param volume     4-bit volume or envelope period (0-15)
 * @param const_vol  true = constant volume, false = use hardware envelope
 * @param loop_env   true = loop envelope / halt length counter
 * @param timer      11-bit timer period (determines pitch)
 * @param length_idx 5-bit length counter load index
 */
void rv2a03_set_pulse1(uint8_t duty, uint8_t volume, bool const_vol, bool loop_env,
                       uint16_t timer, uint8_t length_idx);

/**
 * Configures Pulse (Square) Channel 2.
 */
void rv2a03_set_pulse2(uint8_t duty, uint8_t volume, bool const_vol, bool loop_env,
                       uint16_t timer, uint8_t length_idx);

/**
 * Configures Triangle Channel.
 * @param linear_reload 7-bit linear counter reload value (0-127)
 * @param halt          true = length counter halt / control flag set
 * @param timer         11-bit timer period
 * @param length_idx    5-bit length counter load index
 */
void rv2a03_set_triangle(uint8_t linear_reload, bool halt, uint16_t timer, uint8_t length_idx);

/**
 * Configures Noise Channel.
 * @param volume     4-bit volume or envelope period (0-15)
 * @param const_vol  true = constant volume, false = use envelope
 * @param loop_env   true = loop envelope / halt length counter
 * @param period_idx 4-bit noise frequency table index (0-15, 0 = highest pitch)
 * @param short_mode true = 93-bit metallic noise, false = 32767-bit white noise
 * @param length_idx 5-bit length counter load index
 */
void rv2a03_set_noise(uint8_t volume, bool const_vol, bool loop_env,
                      uint8_t period_idx, bool short_mode, uint8_t length_idx);

// --------------------------------------------------------------------------
// Musical Helpers & Frequency Conversion
// --------------------------------------------------------------------------

/**
 * Computes the 11-bit timer reload value for a Pulse channel given desired frequency in Hz.
 * Timer = (NTSC_CPU_CLOCK / (16 * freq_hz)) - 1
 */
uint16_t rv2a03_calc_pulse_timer(uint32_t freq_hz);

/**
 * Computes the 11-bit timer reload value for the Triangle channel given desired frequency in Hz.
 * Timer = (NTSC_CPU_CLOCK / (32 * freq_hz)) - 1
 */
uint16_t rv2a03_calc_tri_timer(uint32_t freq_hz);

/**
 * Converts a standard MIDI note number (0-127, e.g. 69 = A4 = 440 Hz) to Pulse channel timer period.
 */
uint16_t rv2a03_midi_to_pulse_timer(uint8_t midi_note);

// Convenience note pitch definitions (MIDI note numbers)
#define RV2A03_NOTE_C3   48
#define RV2A03_NOTE_D3   50
#define RV2A03_NOTE_E3   52
#define RV2A03_NOTE_F3   53
#define RV2A03_NOTE_G3   55
#define RV2A03_NOTE_A3   57
#define RV2A03_NOTE_B3   59

#define RV2A03_NOTE_C4   60  // Middle C
#define RV2A03_NOTE_D4   62
#define RV2A03_NOTE_E4   64
#define RV2A03_NOTE_F4   65
#define RV2A03_NOTE_G4   67
#define RV2A03_NOTE_A4   69  // 440 Hz standard pitch
#define RV2A03_NOTE_B4   71

#define RV2A03_NOTE_C5   72
#define RV2A03_NOTE_D5   74
#define RV2A03_NOTE_E5   76
#define RV2A03_NOTE_F5   77
#define RV2A03_NOTE_G5   79
#define RV2A03_NOTE_A5   81
#define RV2A03_NOTE_B5   83

#ifdef __cplusplus
}
#endif
