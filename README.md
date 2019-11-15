# Led controller for STM32F103C6T8 (Blue Pill) #
 
## Requirements ##
 
 Any STM32F103C6T8 board (like Blue Pill or SMT32_Smart) and STM32CubeIDE
 
## Pins ##
  * wsb2812b: connect 5V and GND to your power, and Data to PA7
  
## Connection
One of either:
 * USB's port (/dev/ttyACM0 on Linux; any high baudrate)
 * UART through PA10 (baudrate: 38400, parity: none, stop bits: 1)
  
## Protocol ##
 
Send 1 byte with number of pixels (`n`), then `n` pixel colors (RGB, one byte each).
 
Node code:
```javascript
var Protocol = require('bin-protocol');
var protocol = new Protocol();

protocol.define('frame', {
    write (colors) {
        this.Int8(colors.length)
        for(let c of colors) {
            this.UInt8(c[0])
            this.UInt8(c[1])
            this.UInt8(c[2])
        }
    }
})
//display is array of RGB pixels [0-255]
let colorsData = protocol.write().frame(display).result
port.write(colorsData)
```

## Known problems ##
 * Doesn't work in USB HUBs
 
 Supports up to 120 LEDS 150Hz.