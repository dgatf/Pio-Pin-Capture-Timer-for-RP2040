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

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#define COUNTER_CYCLES 9
typedef enum edge_type_t
{
    EDGE_NONE,
    EDGE_FALL,
    EDGE_RISE
} edge_type_t;
typedef void (*capture_handler_p_t)(uint counter, edge_type_t edge);
static uint sm_capture_edge;
static PIO pio_capture_edge;
static void (*capture_handler_p[PIN_COUNT])(uint counter, edge_type_t edge) = {NULL};
static inline uint capture_edge_init(PIO pio, uint pin_base, float clk_div, uint irq);
static inline void capture_edge_set_irq(uint pin, capture_handler_p_t handler);
static inline void capture_edge_irq();
static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev);
static inline uint bit_value(uint pos);
static inline uint capture_edge_init(PIO pio, uint pin_base, float clk_div, uint irq)
{
    pio_capture_edge = pio;
    sm_capture_edge = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &capture_edge_program);
    pio_sm_set_consecutive_pindirs(pio, sm_capture_edge, pin_base, PIN_COUNT, false);
    pio_sm_config c = capture_edge_program_get_default_config(offset);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_in_pins(&c, pin_base);
    if (irq == PIO0_IRQ_0 || irq == PIO1_IRQ_0)
        pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + IRQ_NUM), true);
    else
        pio_set_irq1_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + IRQ_NUM), true);
    pio_interrupt_clear(pio, IRQ_NUM);
    pio_sm_init(pio, sm_capture_edge, offset + capture_edge_offset_start, &c);
    pio_sm_set_enabled(pio, sm_capture_edge, true);
    irq_set_exclusive_handler(irq, capture_edge_irq);
    irq_set_enabled(irq, true);
    return sm_capture_edge;
}
static inline void capture_edge_irq()
{
    static uint counter_prev = 0;
    if (pio_sm_is_rx_fifo_full(pio_capture_edge, sm_capture_edge))
    {
        pio_sm_clear_fifos(pio_capture_edge, sm_capture_edge);
        return;
    }
    uint counter = pio_sm_get_blocking(pio_capture_edge, sm_capture_edge);
    uint pins = pio_sm_get_blocking(pio_capture_edge, sm_capture_edge);
    uint prev = pio_sm_get_blocking(pio_capture_edge, sm_capture_edge);
    for (uint pin = 0; pin < PIN_COUNT; pin++)
    {
        edge_type_t edge = get_captured_edge(pin, pins, prev);
        if (edge && *capture_handler_p[pin])
        {
            capture_handler_p[pin](counter, edge);
        }
    }
    pio_interrupt_clear(pio_capture_edge, IRQ_NUM);
}
static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev)
{
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && (bit_value(pin) & pins))
        return EDGE_RISE;
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && !(bit_value(pin) & pins))
        return EDGE_FALL;
    return EDGE_NONE;
}
static inline void capture_edge_set_irq(uint pin, capture_handler_p_t handler)
{
    if (pin < PIN_COUNT)
    {
        capture_handler_p[pin] = handler;
    }
}
static inline uint bit_value(uint pos)
{
    return 1 << pos;
}

#endif
