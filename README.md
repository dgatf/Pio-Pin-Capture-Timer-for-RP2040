## Pin Capture Timer for RP2040

A pio program to capture signal edges on any pins of the RP2040. It is intended to be used with the SDK, not Arduino.

How to use it:

- Add *capture_edge.pio* to your project
- Modify CMakeLists.txt. Add *pico_generate_pio_header* and the required libraries (pico_stdlib, hardware_irq, hardware_pio, hardware_clocks). See [CMakeLists.txt](src/CMakeLists.txt)

In *capture_edge.pio* define the number of pins to capture with PIN_COUNT (2 places, first for pioasm and second for GCC). Capture pins starts at *pin_base*. All available pins can be captured.

See [main.c](src/main.c) with code example to calculate *frecuency* and *duty*. Counter increments every 17 clock cycles. This value is defined in COUNTER_CYCLES. To obtain total clock divisor multiply: 17 * *clk_div*. 

Functions:

**uint capture_edge_init(PIO pio, uint pin_base, float div)**

Parameters:  
&nbsp;&nbsp;**pio**      load the capture program at pio0 or pio1  
&nbsp;&nbsp;**pin_base** set the first capture pin  
&nbsp;&nbsp;**clk_div**  set the clock divisor  

Return:  
State machine number used  

**capture_edge_set_irq(uint pin, capture_handler_t handler)**

Parameters:
&nbsp;&nbsp;**pin**      pin to capture  
&nbsp;&nbsp;**handler**  function to handle the capture edge interrupt  

The accuracy is 34 clock cycles (17 cycles per edge).

For a clock frecuency of 125Mhz, accuracy is 0.272 μs