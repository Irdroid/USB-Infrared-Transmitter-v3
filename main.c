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
#include "common.h"
#include "oled_term.h"
// here we save the pulse-space values
// of the captured IR signal
__xdata uint8_t dbg_buff[20];
uint16_t pulse;
uint16_t space;
/** The CDC EP2 write pointer */
extern volatile __xdata uint8_t CDC_writePointer;

uint8_t * cdc_In_buffer_main = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 

bool rx_c;

/** Timer0 is configured to automatically start when the INT0 is high
 *  and TR0=1; In the INT0 ISR, we zero timer0 TH0 and TL0, zero T2H and
 * T2L and we start timer2 and write TR0 = 1, when INT0 becomes high,
 * Timer0 is automatically started (via the INT0 gate configuration for
 * timer0, when we have another INT0 interrupt, we read and save timer0
 * TH0 and TL0, read and save T2H and T2L. The IR pulse is T2H|T2L - TH0|TL0,
 * and the ir space is TH0|TL0)
 * 
 * The signal from the IR Receiver is active-low.
 * 
 * IR Signal(INT0)_______         ______________
 *                      |        |              |
 *                      |        |              |
 *                      | Pulse  |    Space     |
 *                      |________|              |
 *                   (T2_ON)  (T0_ON)    (T2_OFF)(T0_OFF)
 *  
 *  The width of the pulse and space periods are calculated,
 *  using the formula below:
 * 
 *  Space = (TH0 << 8) | TL0
 *  Pulse = ((TH2 << 8) | TL2) - Space    
 */                  
void ext0_interrupt(void) __interrupt(INT_NO_INT0)
{
    // If timer0 and timer2 are not turned on , turn them on
    // this is the initial condition
    if(TR0 == 0 && TR2 == 0){
      // Zero Timer0 and Timer2 high and low regs
      TH2 = 0;
      TL2 = 0;
      TH0=0;
      TL0 = 0;
      // Prepare Timer0 to start when INT0 is low
      TR0 = 1;
      // Start Timer2
      TR2 = 1;
    }else{
      // Stop timer0
      TR0 = 0;
      // Stop timer2
      TR2 = 0;
      // Read Timer0 readings , this is the IR signal space
      space = (TH0 << 8) | TL0;
      // Read Timer2 readings and substact space to get the pulse value
      pulse = ((TH2 << 8) | TL2) - space;
      rx_c = true;
      // Now we have to zero TH0, TL0, TH2 and TL2
      TH0 = 0;
      TL0 = 0;
      TH2 = 0;
      TL2 = 0;
      // And now we have to write TR0 = 1 and TR2 = 1;
      // Eg. preparing for the next pulse-space cycle
      // Timer0 will start after the INT0 is raised,
      // Timer 2 will start immediatly
      TR0=1;
      TR2=1;
    }
}

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
  // If we are here it means that there is no INT0 pin change
  // and we will send any remaining data to the host
  // With the current configuration timer0 will interrupt each 32ms
  WaitInReady();
  CDC_flush(); // flush the buffer
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
  // Setup the timer0 Gated by Int0, when int0 becomes high, timer is started
  TMOD |= bT0_M0 | bT0_GATE ;   /* Run in time mode not counting */ 
  /* By default we are running 24MHz system clock and 2MHz timer clock */
  #ifdef TIMER_CLOCK_FAST
  T2MOD =0b00010000; /* Divide the system clock by 4 */
  #else
  T2MOD =0b00000000; /* Divide the system clock by 12 */
  #endif

	EA  = 1;     /* Enable global interrupt */
  EX0 = 1;    // Enable INT0
  
  /** INT0 is edge triggered, this means that it will be active on
   * each falling edge of the signal comming from the ir receiver
   * */
  IT0 = 1;    // INT0 is edge triggered

  // Main loop
  while(1) {
    // If we have pulse-space measuremnts available, put them in the CDC buffer
    if(rx_c){
      // Divide by this constant to achieve irtoy time unit
      // and report measurements to the host as if it is the irtoy hardware 
      pulse = _divuint(pulse, 43);
      space = _divuint(space, 43);

      *cdc_In_buffer_main++ = (pulse >> 8) & 0xff;
      *cdc_In_buffer_main++ = pulse;
      *cdc_In_buffer_main++ = (space >> 8) & 0xff;
      *cdc_In_buffer_main++ = space;
      DBG("Pulse: %u\n", pulse);
      DBG("Space: %u\n", space);
      while(1);
      CDC_writePointer += sizeof(uint32_t);
      if(CDC_writePointer == MAX_PACKET_SIZE){
        WaitInReady();
        CDC_flush(); // flush the buffer
      }
      rx_c = 0;
    }
  }
}
