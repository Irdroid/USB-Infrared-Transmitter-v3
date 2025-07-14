# Irdroid USB Infrared Transmitter (Hardware version 3.0)

![USB Infrared Transmitter v3](https://[raw.githubusercontent.com/Irdroid/trx_v3/refs/heads/master/Hardware/Pictures/black5%20(Medium).png?token=GHSAT0AAAAAADHKJ2AQDZWZGJRTQG3HPC2Y2DU4MQA](https://raw.githubusercontent.com/Irdroid/USB-Infrared-Transmitter-v3/refs/heads/master/Hardware/Pictures/black3%20(Medium).png))

The Irdroid USB Infrared transmitter device provides infrared remote control capabilities for the host computer. It is supported in Linux , MS Windows and Android. It is capable of controlling appliances such as TVs, STBs, Air Conditioners and basically any device that is controlled via infrared signals.
### Motivation
The first revisions of the Irdroid USB Infrared Transmitter product used Microchip's PIC18F2550 MCU.The increasing cost of this MCU and the limited availability of the MCU, after Covid/2020, demanded the commencement of this project.

### The search for a replacement MCU
During the development of this code a single piece of PIC18F2550 cost around 8USD. So hell-yeah that was a huge motivator.
The search for alternative microcontroller took place years before this project, but no viable candidate was found, until now. The requirements were met by a MCU manufactured by a Chineese company called WCH, and the selected MCU was CH552, a Harward architecture 89c51 core, with all the required peripherals. 
Last but not least, one pcs of the CH552 MCU costs 0.25 USD, providing similar features and performance, so voila...

### The Idea
- Design and implement a USB infrared transmitter adapter, that would be a drop-in replacement for the legacy Irdroid USB Infrared transmitter, without any re-design or re-work to the host drivers. With other words, we had to design and implement the firmware in such way, that the host could't differentiate the legacy and the new device. That turned out to be a challenging task. 
- Design and manufacture a printed circuit board for the new MCU, that would fit into the legacy enclosure, which would further reduce end product cost.
- Take care of the possibility for automatic flashing , during manufacture.
### The Challenges

- Significant differences between the two MCUs in terms of possibilities to configure the system clocks and clock dividers for the timer modules and the PWM module.
    - The Microchip's PIC18f2550, provides greater range of frequency division factors and it is a bit flexible in comparison with the WCH CH55x family of MCUs especially for the PWM module clock configuration.
    - There was a necessity to postprocess the IR data comming from the host to the WCH 552 MCU in order to re-align with its timer configuration for sending the RAW IR data
- Differences in the USB controller in both MCUs. Tweaking in software was required.
    - The implementation on the PIC18F2550 was done using polling the USB controller, rather than using interrupts.
    - The implementation on the CH552 was done using interrupts.
- Different development environment, no debugging capabilities
    - Microchip's PIC18F2550 has debugging support using a Pickit3 ICD adapter
    - The CH55x microcontrollers dont have hardware debugging support, thus in our case debugging was done with redirected printf on a small LCD screen. This is very innacurate in comparison with a hardware debugging support.
- Differences in flashing the firmware to the MCUs
    - The Microchip's PIC18F2550 uses PickIt 3 flash fromgrammers for flashing, they dont come with a bootloader, but pickit3 programmers has a very useful feature for in-field programming - no host required.
    - The WCH CH55x comes with a built in bootloader, which allows for flashing via USB or Serial interface.

### Software Development 

In terms of software, the idea was to re-use the code from the Irdroid USB Infrared Transmitter, which was written in C and port it to the new architecture. CH55x MCUs have a SDK with the SDCC compiler and it can be used with Visual Studio Code, which was our choise for the IDE. 

    - For this MCU there are no debugging capabilities, so we have connected an small i2c OLED display for brief debugging messages with printf.
    - The Saleae Logic analyzer was used also as a tool for debugging and measuring the IR signal output.

### Hardware Design

Due to the fact that we wanted to re-use the already available enclosure, which is used for the previous versions of the Irdroid USB Infrared Transmitter, we have designed the PCB with the same layout and size in order to fit the existing enclosure. For the design of the schematics and the pcb layout we have used EagleCad. A two sides single layer design with FR4 material. The newly designed adapter practically uses almost the same components as in the previous version, and even looks very similar 3 IR LEDs , same enclosure, same USB connector.

### CH552 Hardware
The MCU is 8-bit Harvard architecture 89c51 CPU, 256 bytes of fast RAM and a bit more
than 1k of slower xRam, 3 timer modules, 2 PWM modules, dataflash of 128 Bytes and a hardware USB 2.0 controller.
The above features and its price made it the perfect candidate for the project.

## CH552 SDK
The MCU could be programmed in C and it comes with a SDK (with some examples) for the SDCC compiler. The build system used is GNU Make.

## Instructions for usage of the code

    - Install the SDCC compiler on Linux - apt install sdcc
    - Install vscode and C/C++ plugins and Makefiles plugin for vscode
    - Clone this repository
    - Open a terminal , cd into the source code directory and type code .
    - When vscode loads open a terminal and type "make all", to compile the project
    - In order to flash the binary to the MCU , make sure the ch552 unit is in bootloader mode and type "make flash" in the terminal
    - To clean the project type "make clean"


