/**
 * -------------------------------------------------------------------------------
 * 
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree. 
 * 
 * -------------------------------------------------------------------------------
 * 
 *  Base pin is 7. PIN_COUNT is 2 -> Pins 7 & 8 can be captured
 *
 *  Set the number of pins to capture in capture_edge.pio with PIN_COUNT (note there are two places)
 *
 *  Connect a signal to pins 7 and/or 8 and check output at 115200
 * 
 * -------------------------------------------------------------------------------
 */

#include "hardware/clocks.h"

#include "capture_edge.pio.h"

float clk_div = 1;

static void capture_pin_0_handler(uint counter, edge_type_t edge)
{
    static uint counter_edge_rise = 0, counter_edge_fall = 0;
    static float frequency = 0, duty = 0, duration = 0;

    if (edge == EDGE_RISE)
    {
        duration = (float)(counter - counter_edge_rise) / clock_get_hz(clk_sys) * clk_div * COUNTER_CYCLES;
        frequency = 1 / duration;
        counter_edge_rise = counter;
    }
    if (edge == EDGE_FALL)
    {
        float duration_pulse = (float)(counter - counter_edge_rise) / clock_get_hz(clk_sys) * clk_div * COUNTER_CYCLES;
        duty = duration_pulse / duration * 100;
        counter_edge_fall = counter;
    }
    printf("\nCapture pin 0. Counter: %u Freq: %.1f Duty: %.1f", counter, frequency, duty);
}

static void capture_pin_1_handler(uint counter, edge_type_t edge)
{
    static uint counter_edge_rise = 0, counter_edge_fall = 0;
    static float frequency = 0, duty = 0, duration = 0;

    if (edge == EDGE_RISE)
    {
        duration = (float)(counter - counter_edge_rise) / clock_get_hz(clk_sys) * clk_div * COUNTER_CYCLES;
        frequency = 1 / duration;
        counter_edge_rise = counter;
    }
    if (edge == EDGE_FALL)
    {
        float duration_pulse = (float)(counter - counter_edge_rise) / clock_get_hz(clk_sys) * clk_div * COUNTER_CYCLES;
        duty = duration_pulse / duration * 100;
        counter_edge_fall = counter;
    }
    printf("\nCapture pin 1. Counter: %u Freq: %.1f Duty: %.1f", counter, frequency, duty);
}

int main()
{
    uint sm_capture_edge;
    PIO pio = pio0;
    uint pin_base = 7;

    stdio_init_all();

    sm_capture_edge = capture_edge_init(pio, pin_base, clk_div);
    capture_edge_set_irq(0, capture_pin_0_handler);
    capture_edge_set_irq(1, capture_pin_1_handler);

    while (1)
    {
    }
}
