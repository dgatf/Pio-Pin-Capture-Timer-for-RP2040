/**
   -------------------------------------------------------------------------------

   Copyright (c) 2022, Daniel Gorbea
   All rights reserved.

   This source code is licensed under the MIT-style license found in the
   LICENSE file in the root directory of this source tree.

   -------------------------------------------------------------------------------

   A pio program to capture signal edges on any pins of the RP2040:

   Base pin is 7. CAPTURE_EDGE_PIN_COUNT is 2 -> Pins 7 & 8 can be captured

   Set the number of pins to capture in capture_edge.pio.h with CAPTURE_EDGE_PIN_COUNT

   Connect a signal to pins 7 and/or 8 and check output at 115200

   -------------------------------------------------------------------------------
*/

#include "hardware/clocks.h"

extern "C" {
#include "capture_edge.h"
}

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
  Serial.print("\n\rCapture pin 0. Counter:");
  Serial.print(counter);
  Serial.print(" Freq: ");
  Serial.print(frequency);
  Serial.print(" Duty:");
  Serial.print(duty);
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
  Serial.print("\n\rCapture pin 1. Counter:");
  Serial.print(counter);
  Serial.print(" Freq: ");
  Serial.print(frequency);
  Serial.print(" Duty:");
  Serial.print(duty);
}

void setup()
{
  Serial.begin(115200);

  PIO pio = pio0;        // values: pio0, pio1
  uint sm;               //
  uint pin_base = 7;     //
  uint irq = PIO0_IRQ_0; // values for pio0: PIO0_IRQ_0, PIO0_IRQ_1. values for pio1: PIO1_IRQ_0, PIO1_IRQ_1

  sm = capture_edge_init(pio, pin_base, clk_div, irq);
  capture_edge_set_handler(0, capture_pin_0_handler);
  capture_edge_set_handler(1, capture_pin_1_handler);
}

void loop()
{
}
