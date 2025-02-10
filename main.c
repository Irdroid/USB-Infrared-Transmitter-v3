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
#include "src/irs.h"
#include "src/system.h"                   // system functions
#include "src/delay.h"                    // for delays
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "ch554.h"
#include <stdbool.h>
#include <stdint.h>
#include "usb_cdc.h"
#include "gpio.h"
#define PIN_INT P32
#define PIN_LED P33
#define MAX_SAMPLES 64
uint32_t timer_val1;
uint32_t timer_val2;
/** The CDC EP2 write pointer */
extern volatile __xdata uint8_t CDC_writePointer;
// The CDC In buffer , used to send to the host
uint8_t * cdc_In_buffer_main = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 

#ifdef DEBUG
__xdata uint8_t dbg_buff[20];
#endif

bool rx_c;
bool rx_b;

void ext0_interrupt(void) __interrupt(INT_NO_INT0)
{
    PIN_toggle(PIN_LED);
    // If timer 0 is not turned on , turn it on
    if(TR0 == 0){
      TH0=0;
      TL0 = 0;
      TR0 = 1;
    }else{
      // Check if the previous value was added to the buffer
      // if not save the current value in a separate variable which will be 
      // processed later
      // Turn off timer0
      TR0=0;
      if(rx_c){
      timer_val1 = (TH0 << 8) & 0xff00;
      timer_val1 |= TL0;
      rx_b = true;
      }else{  
      timer_val2 = (TH0 << 8) & 0xff00;
      timer_val2 |= TL0;
      rx_c = true;
      }
      // Zero TH, TL and start the timer 0
      TH0 = 0;
      TL0 = 0;
      TR0 = 1;
    }  
}

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

/** @brief Timer0 Interrupt routine */
void timer0_interrupt(void) __interrupt(INT_NO_TMR0)   
{ 

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
  // this is the interupt pin, the ir receiver
  PIN_output_OD(PIN_INT);  
  // Output pin used to observe the interrupt 0 pin change
  PIN_output(PIN_LED);
  // Setup the timer
  TMOD = 0x1;   /* Run in time mode not counting */ 
  /* By default we are running 24MHz system clock and 2MHz timer clock */
  #ifdef TIMER_CLOCK_FAST
  T2MOD =0b00010000; /* Divide the system clock by 4 */
  #else
  T2MOD =0b00000000; /* Divide the system clock by 12 */
  #endif

	EA  = 1;     /* Enable global interrupt */
  EX0 = 1;    // Enable INT0
  IT0 = 1;    // INT0 is edge triggered

  // Main loop
  while(1) {
    // If we have timer measuremnts available put them in the CDC buffer
    if(rx_c){
      timer_val1 /= TIMER_0_CONST;
      *cdc_In_buffer_main++ = (timer_val1 >> 8) & 0xff;
      *cdc_In_buffer_main++ = timer_val1;
      CDC_writePointer += sizeof(uint16_t);
      if(CDC_writePointer == EP2_SIZE) CDC_flush();
      rx_c = 0;
    }
    // The same as above but used as a backup if we were interrupted before saving the
    // previous timer values
    if(rx_b){
      timer_val2 /= TIMER_0_CONST;
      *cdc_In_buffer_main++ = (timer_val2 >> 8) & 0xff;
      *cdc_In_buffer_main++ = timer_val2;
      CDC_writePointer += sizeof(uint16_t);
      if(CDC_writePointer == EP2_SIZE) CDC_flush();
      rx_b = 0;
    }
  }
}