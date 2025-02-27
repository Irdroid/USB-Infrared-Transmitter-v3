# Port of the Irdroid USB Infrared transceiver to CH552 

## Motivation
The first revisions of the Irdroid USB Infrared Transceiver product targets Microchip's PIC18F2550 MCU, but 
due to the increasing cost of this MCU and the limited availability of the MCU,
this project was started. During the development of this code 1pcs of PIC18F2550 cost around 8USD
and one pcs of the CH552 MCU costs 0.25 USD, providing close features and resources

## CH552 Hardware
The MCU is 8-bit Harvard architecture CPU, 256 bytes of fast RAM and a bit more
than 1k of xRam, 3 timers, 2 PWMs, dataflash of 128Bytes and a USB 2.0 controller.
The above features and its price made it the perfect candidate for the port.

## CH552 SDK
The MCU is programmed in C and it comes with a SDK (with some examples) for the SDCC compiler
The build system used is GNU Make.


