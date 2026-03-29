/*
   -------------------------------------------------------------------------------

   Copyright (c) 2022, Daniel Gorbea
   All rights reserved.

   This source code is licensed under the MIT-style license found in the
   LICENSE file in the root directory of this source tree.

   -------------------------------------------------------------------------------

   A pio program to capture signal edges on any pins of the RP2040:

   Base pin is 0. CAPTURE_EDGE_PIN_COUNT is 2 -> Pins 0 & 1 can be captured

   Set the number of pins to capture in capture_edge.pio.h with CAPTURE_EDGE_PIN_COUNT

   Connect a signal to pins 0 and/or 1 and check output at 115200

   -------------------------------------------------------------------------------
*/

#include "capture_edge.h"
#include "hardware/clocks.h"

float clk_div = 1.0f;

volatile uint capture_counter = 0;
volatile uint pin = 0;
volatile float frequency = 0.0f;
volatile float duty = 0.0f;
volatile float duration_cycle = 0.0f;
volatile float duration_pulse = 0.0f;
volatile bool is_captured = false;
volatile bool period_valid = false;
volatile edge_type_t edge_type = EDGE_NONE;

char msg[200];

static void capture_pin_0_handler(uint counter, edge_type_t edge) {
    static uint counter_edge_rising = 0;

    const float tick_seconds = (float)COUNTER_CYCLES / (float)clock_get_hz(clk_sys);

    capture_counter = counter;
    pin = 0;
    edge_type = edge;

    if (edge == EDGE_RISING) {
        if (counter_edge_rising != 0) {
            duration_cycle = (float)(counter - counter_edge_rising) * tick_seconds;
            if (duration_cycle > 0.0f) {
                frequency = 1.0f / duration_cycle;
                period_valid = true;
            }
        }
        counter_edge_rising = counter;
        is_captured = true;
    } else if (edge == EDGE_FALLING) {
        if (counter_edge_rising != 0) {
            duration_pulse = (float)(counter - counter_edge_rising) * tick_seconds;
            if (period_valid && duration_cycle > 0.0f) {
                duty = duration_pulse / duration_cycle * 100.0f;
            }
            is_captured = true;
        }
    }
}

static void capture_pin_1_handler(uint counter, edge_type_t edge) {
    static uint counter_edge_rising = 0;

    const float tick_seconds = (float)COUNTER_CYCLES / (float)clock_get_hz(clk_sys);

    capture_counter = counter;
    pin = 1;
    edge_type = edge;

    if (edge == EDGE_RISING) {
        if (counter_edge_rising != 0) {
            duration_cycle = (float)(counter - counter_edge_rising) * tick_seconds;
            if (duration_cycle > 0.0f) {
                frequency = 1.0f / duration_cycle;
                period_valid = true;
            }
        }
        counter_edge_rising = counter;
        is_captured = true;
    } else if (edge == EDGE_FALLING) {
        if (counter_edge_rising != 0) {
            duration_pulse = (float)(counter - counter_edge_rising) * tick_seconds;
            if (period_valid && duration_cycle > 0.0f) {
                duty = duration_pulse / duration_cycle * 100.0f;
            }
            is_captured = true;
        }
    }
}

void setup() {
    Serial.begin(115200);

    PIO pio = pio0;         // values: pio0, pio1
    uint pin_base = 0;      // starting gpio to capture
    uint pin_count = 2;     // gpios count
    uint irq = PIO0_IRQ_0;  // values for pio0: PIO0_IRQ_0, PIO0_IRQ_1. values for pio1: PIO1_IRQ_0, PIO1_IRQ_1

    capture_edge_init(pio, pin_base, pin_count, clk_div, irq);
    capture_edge_set_handler(0, capture_pin_0_handler);
    capture_edge_set_handler(1, capture_pin_1_handler);
}

void loop() {
    if (is_captured) {
        uint local_capture_counter;
        uint local_pin;
        float local_frequency;
        float local_duty;
        float local_duration_cycle;
        float local_duration_pulse;
        bool local_period_valid;
        edge_type_t local_edge_type;

        local_capture_counter = capture_counter;
        local_pin = pin;
        local_frequency = frequency;
        local_duty = duty;
        local_duration_cycle = duration_cycle;
        local_duration_pulse = duration_pulse;
        local_period_valid = period_valid;
        local_edge_type = edge_type;
        is_captured = false;

        if (local_edge_type == EDGE_RISING) {
            sprintf(msg, "\r\nCapture pin %u. Counter: %u Edge: Rising",
                    local_pin, local_capture_counter);
            Serial.print(msg);

            if (local_period_valid) {
                sprintf(msg, " Period(us): %.0f Freq(Hz): %.1f Duty: %.1f",
                        local_duration_cycle * 1000000.0f,
                        local_frequency,
                        local_duty);
                Serial.print(msg);
            } else {
                Serial.print(" Period: n/a");
            }
        } else if (local_edge_type == EDGE_FALLING) {
            sprintf(msg, "\r\nCapture pin %u. Counter: %u Edge: Falling PulseHigh(us): %.0f",
                    local_pin, local_capture_counter, local_duration_pulse * 1000000.0f);
            Serial.print(msg);

            if (local_period_valid) {
                sprintf(msg, " Duty: %.1f", local_duty);
                Serial.print(msg);
            }
        }
    }
}