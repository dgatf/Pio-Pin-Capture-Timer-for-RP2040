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
#define COUNTER_CYCLES 5

// ------------ //
// capture_edge //
// ------------ //

#define capture_edge_wrap_target 2
#define capture_edge_wrap 6

#define capture_edge_offset_start 2u

static const uint16_t capture_edge_program_instructions[] = {
    0xc000+CAPTURE_EDGE_IRQ_NUM, //  0: irq    nowait 2                   
    0x8020, //  1: push   block                      
            //     .wrap_target
    0xa041, //  2: mov    y, x                       
    0x4000+CAPTURE_EDGE_PIN_COUNT, //  3: in     pins, 2                    
    0x4060+32-CAPTURE_EDGE_PIN_COUNT, //  4: in     null, 30                   
    0xa026, //  5: mov    x, isr                     
    0x00a0, //  6: jmp    x != y, 0                  
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program capture_edge_program = {
    .instructions = capture_edge_program_instructions,
    .length = 7,
    .origin = -1,
};

static inline pio_sm_config capture_edge_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + capture_edge_wrap_target, offset + capture_edge_wrap);
    return c;
}
#endif
