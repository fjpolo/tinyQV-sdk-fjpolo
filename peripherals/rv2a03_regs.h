/*
 * SPDX-License-Identifier: Apache-2.0
 * RV2A03 NES APU Sound Peripheral Register Definitions for TinyQV
 * Target: Tiny Tapeout Sky25a (Berzerk instance)
 */

#pragma once

#include <stdint.h>
#include "gpio.h"

#ifndef RV2A03_PERIPHERAL_NUM
#define RV2A03_PERIPHERAL_NUM       14
#endif

#define RV2A03_BASE                 ((volatile uint8_t*)PERI_BASE_ADDRESS(RV2A03_PERIPHERAL_NUM))

// --------------------------------------------------------------------------
// Register Offsets (Direct APU Mapping: 0x00 - 0x17)
// --------------------------------------------------------------------------

// Pulse Channel 1
#define RV2A03_REG_SQ1_VOL          0x00    // Duty, loop env, const vol, volume
#define RV2A03_REG_SQ1_SWEEP        0x01    // Sweep unit: enabled, period, negate, shift
#define RV2A03_REG_SQ1_LO           0x02    // Timer period low 8-bits
#define RV2A03_REG_SQ1_HI           0x03    // Length counter load, timer high 3-bits

// Pulse Channel 2
#define RV2A03_REG_SQ2_VOL          0x04    // Duty, loop env, const vol, volume
#define RV2A03_REG_SQ2_SWEEP        0x05    // Sweep unit: enabled, period, negate, shift
#define RV2A03_REG_SQ2_LO           0x06    // Timer period low 8-bits
#define RV2A03_REG_SQ2_HI           0x07    // Length counter load, timer high 3-bits

// Triangle Channel
#define RV2A03_REG_TRI_LINEAR       0x08    // Length halt / linear counter reload
#define RV2A03_REG_TRI_LO           0x0A    // Timer period low 8-bits
#define RV2A03_REG_TRI_HI           0x0B    // Length counter load, timer high 3-bits

// Noise Channel
#define RV2A03_REG_NOISE_VOL        0x0C    // Loop env, const vol, volume
#define RV2A03_REG_NOISE_LO         0x0E    // Mode (random loop), period index [3:0]
#define RV2A03_REG_NOISE_HI         0x0F    // Length counter load

// DMC (Delta Modulation Channel)
#define RV2A03_REG_DMC_FREQ         0x10    // IRQ enable, loop, rate index
#define RV2A03_REG_DMC_RAW          0x11    // Direct 7-bit DAC value
#define RV2A03_REG_DMC_START        0x12    // Sample address: %11AAAAAA.AA000000
#define RV2A03_REG_DMC_LEN          0x13    // Sample length: %LLLL.LLLL0001

// APU Control & Frame Counter
#define RV2A03_REG_STATUS           0x15    // Channel enable (W) / Status (R)
#define RV2A03_REG_FRAME_CNT        0x17    // 4-step/5-step mode, IRQ inhibit

// --------------------------------------------------------------------------
// Extended Peripheral Registers (TinyQV Integration: 0x20 - 0x25)
// --------------------------------------------------------------------------
#define RV2A03_REG_CONFIG0          0x20    // [2]=isMMC5, [1]=US (ultrasound), [0]=CE (clock enable)
#define RV2A03_REG_STATUS0          0x22    // [1]=IRQ, [0]=Data Output Ready
#define RV2A03_REG_DATA_IN          0x23    // Command / DIN write port
#define RV2A03_REG_OUTPUT_MSB       0x24    // Mixed audio sample [15:8] (signed 16-bit)
#define RV2A03_REG_OUTPUT_LSB       0x25    // Mixed audio sample [7:0]

// --------------------------------------------------------------------------
// Bitfield Masks & Constants
// --------------------------------------------------------------------------

// Status Register (0x15) Channel Enables
#define RV2A03_STATUS_SQ1_ENABLE    (1 << 0)
#define RV2A03_STATUS_SQ2_ENABLE    (1 << 1)
#define RV2A03_STATUS_TRI_ENABLE    (1 << 2)
#define RV2A03_STATUS_NOISE_ENABLE  (1 << 3)
#define RV2A03_STATUS_DMC_ENABLE    (1 << 4)
#define RV2A03_STATUS_ALL_ENABLE    (RV2A03_STATUS_SQ1_ENABLE | \
                                     RV2A03_STATUS_SQ2_ENABLE | \
                                     RV2A03_STATUS_TRI_ENABLE | \
                                     RV2A03_STATUS_NOISE_ENABLE)

// Configuration 0 (0x20)
#define RV2A03_CFG_CE               (1 << 0)    // Clock Enable
#define RV2A03_CFG_US               (1 << 1)    // Allow Ultrasound
#define RV2A03_CFG_MMC5             (1 << 2)    // Enable MMC5 expansion

// Pulse Duty Cycles
#define RV2A03_DUTY_12_5            (0 << 6)    // 12.5% duty cycle
#define RV2A03_DUTY_25              (1 << 6)    // 25.0% duty cycle
#define RV2A03_DUTY_50              (2 << 6)    // 50.0% duty cycle
#define RV2A03_DUTY_75              (3 << 6)    // 75.0% negated duty cycle

// Envelope & Volume Flags
#define RV2A03_ENV_CONST_VOL        (1 << 4)    // Disable envelope, use constant volume
#define RV2A03_ENV_LOOP             (1 << 5)    // Loop envelope / halt length counter

// Noise Mode Flags
#define RV2A03_NOISE_MODE_32767     (0 << 7)    // 15-bit long LFSR (white noise)
#define RV2A03_NOISE_MODE_93        (1 << 7)    // 93-bit short LFSR (metallic buzz)

// Triangle Flags
#define RV2A03_TRI_CONTROL_HALT     (1 << 7)    // Control flag / length counter halt
