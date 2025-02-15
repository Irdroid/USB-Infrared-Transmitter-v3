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
#include "src/system.h"                   // system functions
#include "src/delay.h"                    // for delays
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "ch554.h"
#include <stdbool.h>
#include <stdint.h>
#include "usb_cdc.h"
#include "gpio.h"
#include "common.h"
#define PIN_INT P32
#define PIN_LED P33
#define MAX_SAMPLES 64

#ifdef DEBUG
__xdata uint8_t dbg_buff[20];
#endif

uint_fast16_t timer_val;

/** The CDC EP2 write pointer */
extern volatile __bit CDC_EP2_readPointer;
extern volatile __xdata uint8_t CDC_writePointer;
uint8_t * cdc_In_buffer = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 
uint8_t * cdc_Out_buffer = (uint8_t *) EP2_buffer; 
uint8_t rx_samples;
uint8_t h,l,h1,l1;

bool rx_c;
bool rx_b;

int cnt =0;
uint_fast16_t first;
uint_fast16_t last;
int edge = 0;
int count = 0;

void t2(void) __interrupt(INT_NO_TMR2)
{
    if(CAP1F == 0 || EXF2 ==0){
      // timer has timed out
      TR2 = 0;
    }
    // If timer 2 is not on turn it on
    if(TR2 == 0);TR2 = 1;
    // Check if this is the first captured edge
    if( edge == 0){
      first = (T2CAP1H << 8) & 0xff00 | T2CAP1L;
      edge++;
      CAP1F = 0;
    }else if(edge == 1){
      last = (T2CAP1H << 8) & 0xff00 | T2CAP1L;
      if(!rx_c){
        timer_val = (last-first);
        rx_c = true;
        edge = 0;
        CAP1F = 0;
        return;
      }else{
        // overflow
        DBG("ovf\n");
        while(1);
      }
    }
}

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
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
  PIN_input(P14);  
  // Output pin used to observe the interrupt 0 pin change
  PIN_output(PIN_LED);
 
  // Setup the timer
  T2CON = 0b00001001;   /* Run in time mode not counting */ 
  /* By default we are running 24MHz system clock and 2MHz timer clock */
  #ifdef TIMER_CLOCK_FAST
  T2MOD =0b00010000; /* Divide the system clock by 4 */
  #else
  T2MOD = bT2_CAP_M0|bT2_CAP1_EN; // Configure timer 2 in capture mode, pin P14 activates it
  #endif
  PIN_FUNC |= bT2_PIN_X;
	ET2  = 1;     /* Enable global interrupt */
  TR2 = 0;

  rx_samples = 0;
  bool buffer_sent = false;
  int i = 0;
  // Main loop
  while(1) {
    // If we have timer measuremnts available put them in the CDC buffer
    if(rx_c){
      while(CDC_writeBusyFlag);                       // wait for ready to write
      cdc_In_buffer[i] =(timer_val >> 8) & 0xff;        // write character to buffer
      i++;
      CDC_writePointer++;
      if(CDC_writePointer == MAX_PACKET_SIZE){
        CDC_flush();
        i= 0;
      }
      while(CDC_writeBusyFlag);                       // wait for ready to write
      cdc_In_buffer[i] = timer_val ;        // write character to buffer
      i++;
      CDC_writePointer++;
      if(CDC_writePointer == MAX_PACKET_SIZE) {
        CDC_flush();
      i=0;}
      rx_c = false;
    }
  }
}
