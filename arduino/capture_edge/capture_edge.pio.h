/*
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree. 
 *
 * Library for pin capture timer for RP2040
 */

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

#define CAPTURE_EDGE_PIN_COUNT 2
#define CAPTURE_EDGE_IRQ_NUM 2
#define COUNTER_CYCLES 9

// ------------ //
// capture_edge //
// ------------ //

#define capture_edge_wrap_target 10
#define capture_edge_wrap 19

#define capture_edge_offset_start 19u

static const uint16_t capture_edge_program_instructions[] = {
    0xa0e6, //  0: mov    osr, isr                   
    0xa0ce, //  1: mov    isr, !isr                  
    0x8000, //  2: push   noblock                    
    0xa0c1, //  3: mov    isr, x                     
    0x8000, //  4: push   noblock                    
    0xa047, //  5: mov    y, osr                     
    0xa0e1, //  6: mov    osr, x                     
    0xc000+CAPTURE_EDGE_IRQ_NUM, //  7: irq    nowait 2                   
    0x0089, //  8: jmp    y--, 9                     
    0x038a, //  9: jmp    y--, 10                [3] 
            //     .wrap_target
    CAPTURE_EDGE_PIN_COUNT+0x4000,    // 12: in     pins, CAPTURE_EDGE_PIN_COUNT                    
    32-CAPTURE_EDGE_PIN_COUNT+0x4060, // 13: in     null, 32-CAPTURE_EDGE_PIN_COUNT                 
    0xa026, // 12: mov    x, isr                     
    0xa0c2, // 13: mov    isr, y                     
    0xa047, // 14: mov    y, osr                     
    0x00a0, // 15: jmp    x != y, 0                  
    0xa046, // 16: mov    y, isr                     
    0xa0e1, // 17: mov    osr, x                     
    0x008a, // 18: jmp    y--, 10                    
    0xa04b, // 19: mov    y, !null                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program capture_edge_program = {
    .instructions = capture_edge_program_instructions,
    .length = 20,
    .origin = -1,
};

static inline pio_sm_config capture_edge_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + capture_edge_wrap_target, offset + capture_edge_wrap);
    return c;
}
#endif

