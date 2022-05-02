/**
   -------------------------------------------------------------------------------

   Copyright (c) 2022, Daniel Gorbea
   All rights reserved.

   This source code is licensed under the MIT-style license found in the
   LICENSE file in the root directory of this source tree.

   -------------------------------------------------------------------------------
*/

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

#define PIN_COUNT 2
#define IRQ_NUM 0
#define COUNTER_CYCLES 9

// ------------ //
// capture_edge //
// ------------ //

#define capture_edge_wrap_target 12
#define capture_edge_wrap 21

#define capture_edge_offset_start 21u

static const uint16_t capture_edge_program_instructions[] = {
    0xa0e6, //  0: mov    osr, isr                   
    0xa0ce, //  1: mov    isr, !isr                  
    0x8000, //  2: push   noblock                    
    0xa0c1, //  3: mov    isr, x                     
    0x8000, //  4: push   noblock                    
    0xa0c2, //  5: mov    isr, y                     
    0x8000, //  6: push   noblock                    
    0xa047, //  7: mov    y, osr                     
    0xa0e1, //  8: mov    osr, x                     
    0xc000+IRQ_NUM, //  9: irq    nowait 0                   
    0x008b, // 10: jmp    y--, 11                    
    0x008c, // 11: jmp    y--, 12                    
            //     .wrap_target
    PIN_COUNT+0x4000,    // 12: in     pins, PIN_COUNT                    
    32-PIN_COUNT+0x4060, // 13: in     null, 32-PIN_COUNT                 
    0xa026, // 14: mov    x, isr                     
    0xa0c2, // 15: mov    isr, y                     
    0xa047, // 16: mov    y, osr                     
    0x00a0, // 17: jmp    x != y, 0                  
    0xa046, // 18: mov    y, isr                     
    0xa0e1, // 19: mov    osr, x                     
    0x008c, // 20: jmp    y--, 12                    
    0xa04b, // 21: mov    y, !null                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program capture_edge_program = {
    .instructions = capture_edge_program_instructions,
    .length = 22,
    .origin = -1,
};

static inline pio_sm_config capture_edge_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + capture_edge_wrap_target, offset + capture_edge_wrap);
    return c;
}
#endif

