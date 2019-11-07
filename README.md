 # Led controller for STM32F103C6T8 (Blue Pill) #
 
 ## Requirements ##
 
 Any STM32F103C6T8 board (like Blue Pill or SMT32_Smart) and STM32CubeIDE
 
 ## Pins ##
  * wsb2812b: connect 5V and GND to your power, and Data to PA7
  * any UART's TX to PA10
  
 ## Protocol ##
 
 Send 1 byte with number of pixels (`n`), then `n` pixel colors (RGB, one byte each).
 
 Supports up to 120 LEDS.