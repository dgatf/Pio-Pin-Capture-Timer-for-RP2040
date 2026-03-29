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

#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/irq.h"

#define MAX_PIN_COUNT 2

static volatile uint pins_, counter_;
static const uint reload_counter_ = 0xffffffff;
static const uint reload_pins_ = 1;

static uint sm_, offset_;
static uint dma_channel_write_pins_, dma_channel_write_counter_, dma_channel_counter_;
static uint dma_channel_reload_counter_, dma_channel_reload_pins_, dma_channel_reload_write_counter_;
static uint dma_timer_;
static uint pin_count_, irq_;

static bool initialized_ = false;
static PIO pio_;
static uint prev_pins_ = 0;  // previous pin state for edge detection; reset on each init
static void (*handler_[MAX_PIN_COUNT])(uint counter, edge_type_t edge) = {NULL};

static inline void handler_pio(void);
static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev);
static inline uint bit_value(uint pos);

void capture_edge_init(PIO pio, uint pin_base, uint pin_count, float clk_div, uint irq) {
    if (initialized_) {
        capture_edge_remove();
    }

    if (pin_count == 0 || pin_count > MAX_PIN_COUNT) {
        panic("capture_edge_init: invalid pin_count");
    }

    pio_ = pio;
    pin_count_ = pin_count;
    irq_ = irq;
    pins_ = 0;
    counter_ = 0;
    prev_pins_ = 0;

    for (uint pin = 0; pin < MAX_PIN_COUNT; pin++) {
        handler_[pin] = NULL;
    }

    // Claim state machine and load PIO program
    sm_ = pio_claim_unused_sm(pio_, true);
    offset_ = pio_add_program(pio_, &capture_edge_program);

    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_base, pin_count, false);

    pio_sm_config c = capture_edge_program_get_default_config(offset_);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_in_pins(&c, pin_base);

    // Patch IN instruction width to match selected pin count
    pio_->instr_mem[offset_ + 3] = pio_encode_in(pio_pins, pin_count);
    pio_->instr_mem[offset_ + 4] = pio_encode_in(pio_null, 32 - pin_count);

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    if (irq == PIO0_IRQ_0 || irq == PIO1_IRQ_0) {
        pio_set_irq0_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), true);
    } else {
        pio_set_irq1_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), true);
    }

    pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);
    pio_sm_init(pio_, sm_, offset_ + capture_edge_offset_start, &c);

    irq_set_exclusive_handler(irq_, handler_pio);
    irq_set_enabled(irq_, true);

    // Claim DMA channels
    dma_channel_write_pins_ = dma_claim_unused_channel(true);
    dma_channel_write_counter_ = dma_claim_unused_channel(true);
    dma_channel_counter_ = dma_claim_unused_channel(true);
    dma_channel_reload_counter_ = dma_claim_unused_channel(true);
    dma_channel_reload_pins_ = dma_claim_unused_channel(true);
    dma_channel_reload_write_counter_ = dma_claim_unused_channel(true);

    // DMA channel: write captured pins from PIO RX FIFO into pins_
    dma_channel_config config_dma_channel_write_pins = dma_channel_get_default_config(dma_channel_write_pins_);
    channel_config_set_transfer_data_size(&config_dma_channel_write_pins, DMA_SIZE_32);
    channel_config_set_write_increment(&config_dma_channel_write_pins, false);
    channel_config_set_read_increment(&config_dma_channel_write_pins, false);
    channel_config_set_dreq(&config_dma_channel_write_pins, pio_get_dreq(pio_, sm_, false));
    channel_config_set_chain_to(&config_dma_channel_write_pins, dma_channel_reload_pins_);
    dma_channel_configure(dma_channel_write_pins_, &config_dma_channel_write_pins,
                          &pins_,           // write address
                          &pio_->rxf[sm_],  // read address
                          1, false);

    // DMA channel: snapshot counter DMA transfer_count into counter_
    dma_channel_config config_dma_channel_write_counter = dma_channel_get_default_config(dma_channel_write_counter_);
    channel_config_set_transfer_data_size(&config_dma_channel_write_counter, DMA_SIZE_32);
    channel_config_set_write_increment(&config_dma_channel_write_counter, false);
    channel_config_set_read_increment(&config_dma_channel_write_counter, false);
    channel_config_set_dreq(&config_dma_channel_write_counter, pio_get_dreq(pio_, sm_, false));
    channel_config_set_chain_to(&config_dma_channel_write_counter, dma_channel_reload_write_counter_);
    dma_channel_configure(dma_channel_write_counter_, &config_dma_channel_write_counter,
                          &counter_,                                         // write address
                          &dma_hw->ch[dma_channel_counter_].transfer_count,  // read address
                          1, false);

    // DMA channel: free-running counter using DMA timer pacing
    dma_timer_ = dma_claim_unused_timer(true);
    dma_timer_set_fraction(dma_timer_, 1, COUNTER_CYCLES);

    dma_channel_config config_dma_channel_counter = dma_channel_get_default_config(dma_channel_counter_);
    channel_config_set_write_increment(&config_dma_channel_counter, false);
    channel_config_set_read_increment(&config_dma_channel_counter, false);
    channel_config_set_dreq(&config_dma_channel_counter, dma_get_timer_dreq(dma_timer_));
    channel_config_set_chain_to(&config_dma_channel_counter, dma_channel_reload_counter_);
    dma_channel_configure(dma_channel_counter_, &config_dma_channel_counter,
                          NULL,  // write address
                          NULL,  // read address
                          0xffffffff, false);

    // DMA channel: reload counter transfer count
    dma_channel_config config_dma_channel_reload_counter = dma_channel_get_default_config(dma_channel_reload_counter_);
    channel_config_set_transfer_data_size(&config_dma_channel_reload_counter, DMA_SIZE_32);
    channel_config_set_write_increment(&config_dma_channel_reload_counter, false);
    channel_config_set_read_increment(&config_dma_channel_reload_counter, false);
    dma_channel_configure(dma_channel_reload_counter_, &config_dma_channel_reload_counter,
                          &dma_hw->ch[dma_channel_counter_].al1_transfer_count_trig,  // write address
                          &reload_counter_,                                           // read address
                          1, false);

    // DMA channel: reload write_pins transfer count
    dma_channel_config config_dma_channel_reload_pins = dma_channel_get_default_config(dma_channel_reload_pins_);
    channel_config_set_transfer_data_size(&config_dma_channel_reload_pins, DMA_SIZE_32);
    channel_config_set_write_increment(&config_dma_channel_reload_pins, false);
    channel_config_set_read_increment(&config_dma_channel_reload_pins, false);
    dma_channel_configure(dma_channel_reload_pins_, &config_dma_channel_reload_pins,
                          &dma_hw->ch[dma_channel_write_pins_].al1_transfer_count_trig,  // write address
                          &reload_pins_,                                                 // read address
                          1, false);

    // DMA channel: reload write_counter transfer count
    dma_channel_config config_dma_channel_reload_write_counter =
        dma_channel_get_default_config(dma_channel_reload_write_counter_);
    channel_config_set_transfer_data_size(&config_dma_channel_reload_write_counter, DMA_SIZE_32);
    channel_config_set_write_increment(&config_dma_channel_reload_write_counter, false);
    channel_config_set_read_increment(&config_dma_channel_reload_write_counter, false);
    dma_channel_configure(dma_channel_reload_write_counter_, &config_dma_channel_reload_write_counter,
                          &dma_hw->ch[dma_channel_write_counter_].al1_transfer_count_trig,  // write address
                          &reload_pins_,                                                    // read address
                          1, false);

    dma_start_channel_mask((1u << dma_channel_write_pins_) | (1u << dma_channel_write_counter_) |
                           (1u << dma_channel_counter_));

    pio_sm_set_enabled(pio_, sm_, true);
    initialized_ = true;
}

void capture_edge_set_handler(uint pin, capture_handler_t handler) {
    if (pin < pin_count_) {
        handler_[pin] = handler;
    }
}

void capture_edge_remove(void) {
    if (!initialized_) {
        return;
    }

    // Disable IRQ generation from PIO
    if (irq_ == PIO0_IRQ_0 || irq_ == PIO1_IRQ_0) {
        pio_set_irq0_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), false);
    } else {
        pio_set_irq1_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + CAPTURE_EDGE_IRQ_NUM), false);
    }

    irq_set_enabled(irq_, false);
    irq_remove_handler(irq_, handler_pio);
    pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);

    // Stop state machine
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_clear_fifos(pio_, sm_);

    // Abort DMA activity before unclaiming channels
    dma_channel_abort(dma_channel_write_pins_);
    dma_channel_abort(dma_channel_write_counter_);
    dma_channel_abort(dma_channel_counter_);
    dma_channel_abort(dma_channel_reload_counter_);
    dma_channel_abort(dma_channel_reload_pins_);
    dma_channel_abort(dma_channel_reload_write_counter_);

    for (uint pin = 0; pin < MAX_PIN_COUNT; pin++) {
        handler_[pin] = NULL;
    }

    dma_channel_unclaim(dma_channel_write_pins_);
    dma_channel_unclaim(dma_channel_write_counter_);
    dma_channel_unclaim(dma_channel_counter_);
    dma_channel_unclaim(dma_channel_reload_counter_);
    dma_channel_unclaim(dma_channel_reload_pins_);
    dma_channel_unclaim(dma_channel_reload_write_counter_);
    dma_timer_unclaim(dma_timer_);

    pio_remove_program(pio_, &capture_edge_program, offset_);
    pio_sm_unclaim(pio_, sm_);

    pins_ = 0;
    counter_ = 0;
    pin_count_ = 0;
    initialized_ = false;
}

static inline void handler_pio(void) {
    pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);

    uint counter = ~counter_;
    uint pins = pins_;

    for (uint pin = 0; pin < pin_count_; pin++) {
        edge_type_t edge = get_captured_edge(pin, pins, prev_pins_);
        if (handler_[pin] && edge) {
            handler_[pin](counter, edge);
        }
    }

    prev_pins_ = pins;
}

static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev) {
    if (((bit_value(pin) & pins) ^ (bit_value(pin) & prev)) && (bit_value(pin) & pins)) {
        return EDGE_RISING;
    }

    if (((bit_value(pin) & pins) ^ (bit_value(pin) & prev)) && !(bit_value(pin) & pins)) {
        return EDGE_FALLING;
    }

    return EDGE_NONE;
}

static inline uint bit_value(uint pos) { return 1u << pos; }