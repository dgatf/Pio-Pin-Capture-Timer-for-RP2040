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

#include "hardware/clocks.h"
#include "capture_edge.h"

float clk_div = 1;
volatile uint capture_counter, pin;
volatile float frequency, duty, duration_cycle;
volatile bool is_captured;
volatile edge_type_t edge_type;
char msg[200];

static void capture_pin_0_handler(uint counter, edge_type_t edge)
{
    static uint counter_edge_rising = 0, counter_edge_falling = 0;
    capture_counter = counter;
    pin = 0;
    is_captured = true;
    edge_type = edge;

    if (edge == EDGE_RISING)
    {
        duration_cycle = (float)(counter - counter_edge_rising) / clock_get_hz(clk_sys) * COUNTER_CYCLES;
        frequency = 1 / duration_cycle;
        counter_edge_rising = counter;
    }
    if (edge == EDGE_FALLING)
    {
        float duration_pulse = (float)(counter - counter_edge_rising) / clock_get_hz(clk_sys) * COUNTER_CYCLES;
        duty = duration_pulse / duration_cycle * 100;
        counter_edge_falling = counter;
    }
}

static void capture_pin_1_handler(uint counter, edge_type_t edge)
{
    static uint counter_edge_rising = 0, counter_edge_falling = 0;
    capture_counter = counter;
    pin = 1;
    is_captured = true;
    edge_type = edge;

    if (edge == EDGE_RISING)
    {
        duration_cycle = (float)(counter - counter_edge_rising) / clock_get_hz(clk_sys) * COUNTER_CYCLES;
        frequency = 1 / duration_cycle;
        counter_edge_rising = counter;
    }
    if (edge == EDGE_FALLING)
    {
        float duration_pulse = (float)(counter - counter_edge_rising) / clock_get_hz(clk_sys) * COUNTER_CYCLES;
        duty = duration_pulse / duration_cycle * 100;
        counter_edge_falling = counter;
    }
}

void setup()
{
    Serial.begin(115200);

    PIO pio = pio0;        // values: pio0, pio1
    uint pin_base = 0;     // starting gpio to capture
    uint irq = PIO0_IRQ_0; // values for pio0: PIO0_IRQ_0, PIO0_IRQ_1. values for pio1: PIO1_IRQ_0, PIO1_IRQ_1

    capture_edge_init(pio, pin_base, clk_div, irq);
    capture_edge_set_handler(0, capture_pin_0_handler);
    capture_edge_set_handler(1, capture_pin_1_handler);
}

void loop()
{
    if (is_captured)
    {
        sprintf(msg, "\n\rCapture pin %u. Counter: %u State: %s Duration(us): %.0f" , pin, capture_counter, edge_type == EDGE_FALLING ? "High" : "Low ", duration_cycle * 1000000);
        Serial.print(msg);
        if (edge_type == EDGE_RISING)
        {
            sprintf(msg, " Freq(Hz): %.1f Duty: %.1f", frequency, duty);
            Serial.print(msg);
        }
        is_captured = false;
    }
}
