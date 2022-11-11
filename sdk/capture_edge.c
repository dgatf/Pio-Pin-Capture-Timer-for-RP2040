/*
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Library for pin capture timer for RP2040
 */

#include "capture_edge.h"

static uint sm_, offset_;
static PIO pio_;
static void (*handler_[CAPTURE_EDGE_PIN_COUNT])(uint counter, edge_type_t edge) = {NULL};

static inline void handler_pio(void);
static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev);
static inline uint bit_value(uint pos);

uint capture_edge_init(PIO pio, uint pin_base, float clk_div, uint irq)
{
    pio_ = pio;
    sm_ = pio_claim_unused_sm(pio_, true);
    offset_ = pio_add_program(pio_, &capture_edge_program);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_base, CAPTURE_EDGE_PIN_COUNT, false);
    pio_sm_config c = capture_edge_program_get_default_config(offset_);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_in_pins(&c, pin_base);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    if (irq == PIO0_IRQ_0 || irq == PIO1_IRQ_0)
        pio_set_irq0_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), true);
    else
        pio_set_irq1_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), true);
    pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);
    pio_sm_init(pio_, sm_, offset_ + capture_edge_offset_start, &c);
    pio_sm_set_enabled(pio_, sm_, true);
    irq_set_exclusive_handler(irq, handler_pio);
    irq_set_enabled(irq, true);

    return sm_;
}

void capture_edge_set_handler(uint pin, capture_handler_t handler)
{
    if (pin < CAPTURE_EDGE_PIN_COUNT)
    {
        handler_[pin] = handler;
    }
}

void capture_edge_remove(void)
{
    for (uint pin = 0; pin < CAPTURE_EDGE_PIN_COUNT; pin++)
        capture_edge_set_handler(pin, NULL);
    pio_remove_program(pio_, &capture_edge_program, offset_);
    pio_sm_unclaim(pio_, sm_);
}

static inline void handler_pio(void)
{
    static uint prev_pins = 0;
    if (pio_sm_is_rx_fifo_full(pio_, sm_))
    {
        pio_sm_clear_fifos(pio_, sm_);
    }
    else
    {
        while (pio_sm_get_rx_fifo_level(pio_, sm_) > 1)
        {
            uint counter = pio_sm_get_blocking(pio_, sm_);
            uint pins = pio_sm_get_blocking(pio_, sm_);
            for (uint pin = 0; pin < CAPTURE_EDGE_PIN_COUNT; pin++)
            {
                edge_type_t edge = get_captured_edge(pin, pins, prev_pins);
                if (edge && *handler_[pin])
                    handler_[pin](counter, edge);
            }
            prev_pins = pins;
        }
    }
    pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);
}

static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev)
{
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && (bit_value(pin) & pins))
        return EDGE_RISING;
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && !(bit_value(pin) & pins))
        return EDGE_FALLING;
    return EDGE_NONE;
}

static inline uint bit_value(uint pos)
{
    return 1 << pos;
}
