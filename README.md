## Pin Capture Timer for RP2040

A pio program to capture signal edges on any pins of the RP2040. It is compatible with the [SDK](https://raspberrypi.github.io/pico-sdk-doxygen/) and [Arduino](https://github.com/earlephilhower/arduino-pico).

How to use it:

- With SDK. Add *capture_edge.pio* to your project. Modify CMakeLists.txt. Add *pico_generate_pio_header* and the required libraries (pico_stdlib, hardware_irq, hardware_pio, hardware_clocks). See [CMakeLists.txt](src/CMakeLists.txt)
- With Arduino. Add *capture_edge.pio.h* to your project
- Set the number of pins to capture with PIN_COUNT in *capture_edge.pio* or in *capture_edge.pio.h* if using Arduino. Note there are 2 places for PIN_COUNT if using the SDK and 1 place if using Arduino. Capture pins starts at *pin_base*. All available pins can be captured.
- Define the capture handlers which receives the counter value and the edge type (fall or rise).

See [main.c](src/main.c) with code example to calculate *frecuency* and *duty*. Counter increments every 9 clock cycles. This value is defined in COUNTER_CYCLES. To obtain total clock divisor multiply:  COUNTER_CYCLES * *clk_div*. 

Functions:

**uint capture_edge_init(PIO pio, uint pin_base, float div)**  

Parameters:  
&nbsp;&nbsp;**pio** - load the capture program at pio0 or pio1  
&nbsp;&nbsp;**pin_base** - set the first capture pin  
&nbsp;&nbsp;**clk_div** - set the clock divisor  

Return:  
State machine number used  

**void capture_edge_set_irq(uint pin, capture_handler_t handler)**  

Parameters:  
&nbsp;&nbsp;**pin** - pin to capture  
&nbsp;&nbsp;**handler** - function to handle the capture edge interrupt  

Handler functions:  

**void capture_handler(uint counter, edge_type_t edge)**  

Parameters received:  
&nbsp;&nbsp;**counter** - counter   
&nbsp;&nbsp;**edge** - type of edge: EDGE_RISE = 2, EDGE_FALL = 1  

With *clock_div = 1*, the accuracy on frecuency measurement is 18 clock cycles (9 cycles per edge). For a clock frecuency of 125Mhz, accuracy on edge is 0.072 μs and on frecuency is 0.144 μs.

<p align="center"><img src="./images/capture1.png" width="800"><br>  
  <i>Capturing edges with RP2040, verifying with the oscilloscope</i><br><br></p>

<p align="center"><img src="./images/capture2.jpg" width="800"><br>
  <i>Uno is generating the signal, Mega is the oscilloscope and RP2040 is capturing edges</i><br><br></p>