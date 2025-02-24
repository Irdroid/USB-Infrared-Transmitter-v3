// ===================================================================================
// Project:   Irdroid USB Infrared Transceiver for CH551, CH552, CH554
// Version:   v1.0
// Year:      2025
// Author:    Georgi Bakalski
// Github:    https://github.com/irdroid
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// This is a port of the Irdroid USB Infrared Tranceiver firmware from PIC18F2550 to 
// CH551, CH552, CH554
// References:
// -----------
// - Blinkinlabs: https://github.com/Blinkinlabs/ch554_sdcc

// Compilation Instructions:
// -------------------------
// - Chip:  CH551, CH552 or CH554
// - Clock: 24 MHz internal or external
// - Adjust the firmware parameters in src/config.h if necessary.
// - Make sure SDCC toolchain and Python3 with PyUSB is installed.
// - Press BOOT button on the board and keep it pressed while connecting it via USB
//   with your PC.
// - Run 'make flash' immediatly afterwards.
// - To compile the firmware using the Arduino IDE, follow the instructions in the 
//   .ino file.
// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include "src/config.h"                   // user configurations
#include "src/system.h"                   // system functions
#include "src/gpio.h"                     // for GPIO
#include "src/delay.h"                    // for delays
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "src/oled_term.h"                // for OLED
#include "src/irs.h"                      // IR sampling routines
#include "src/hwprofile.h"                  // Hardware profile
#include "src/dataflash.h"
#include "usb_cdc.h"
#include "src/common.h"

#ifdef DEBUG
__xdata uint8_t dbg_buff[20];
#endif
// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

/** @brief Timer0 Interrupt routine */
void timer0_interrupt(void) __interrupt(INT_NO_TMR0)   
{ 
  timer0_int_callback(); 
}

/** @brief Timer1 Interrupt routine */
void timer1_interrupt(void) __interrupt(INT_NO_TMR1)   
{ 
  timer1_int_callback(); 
}

static enum _mode {
    IR_MAIN = 0,
    IR_SUMP, //SUMP logic analyzer
    IR_S, //IR Sampling mode
} mode = IR_MAIN; //mode variable tracks the IR Toy mode

void SetUpDefaultMainMode(void) {
    mode = IR_MAIN; // Main mode
}

// ===================================================================================
// Main Function
// ===================================================================================

void main(void) {
  // Setup
  CLK_config();                           // configure system clock
  DLY_ms(5);                              // wait for clock to stabilize
  CDC_init();                             // init the USB CDC
  #ifdef DEBUG
  OLED_init();                          // Init the oled display/debugging  
  #endif
  SetUpDefaultMainMode();                 // Setup default main mode
  // Main loop
  while(1) {
    
    switch (mode)
    {
      case IR_S:
        if (irsService() != 0) SetUpDefaultMainMode();
        break;
      
      default:
        if(CDC_available()) {       // something coming in?
          
          char byte = CDC_read_b(); // read the character ...  
          
          switch (byte)
          {

              case 'S': //IRIO Sampling Mode
              case 's': 
                  mode = IR_S;
                  irsSetup();
                  break;
              case 'V':
              case 'v':// Acquire Version
                  GetUsbIrdroidVersion();
                  break;
              case 0x00:
                break;
              default:
                break;         
        }
        break;
      }
    }    
  }
}
