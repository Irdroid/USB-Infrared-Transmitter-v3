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
#include "src/hwprofile.h"                // Hardware profile
#include "src/dataflash.h"
#include "usb_cdc.h"
#include "src/common.h"

#ifdef DEBUG
__xdata uint8_t dbg_buff[20];
#endif
extern uint8_t CDC_readPointer;     // data pointer for fetching
extern uint16_t target_freq;
/** References to the CDC in and out buffers */
register uint8_t * cdc_Out_buffer = (uint8_t *) EP2_buffer; 
register uint8_t * cdc_In_buffer = (uint8_t *) EP2_buffer+128; 
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
/** Timer 2 Interrupt Service Routine (Vector 5) */
void Timer2_ISR(void) __interrupt (INT_NO_TMR2) {
  timer2_int_callback(); 
}

/** The EXT0 pin and pin 1.1 are wire together with,
 *  the IR Receiver, so that we have a way to turn on Timer2 
 *  when we have a pin edge change
 */
void ext0_interrupt(void) __interrupt(INT_NO_INT0){
    ENABLE_TIMER2();
    cdc_In_buffer = inWhich(); 
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
  CLK_config();                         // configure system clock
  DLY_ms(10);                           // wait for clock to stabilize
  CDC_init();                           // init the USB CDC  
  #ifdef DEBUG
  OLED_init();                          // Init the oled display/debugging  
  #endif
  SetUpDefaultMainMode();               // Setup default main mode
  PIN_low(PIN_PWM);
  
  /* This gives us information if we got a
   * command to jump to the bootloader */
  if(RST_wasWDT()){
    DBG("SW Reset");
    BOOT_now();
  }
   
/*
    * PWM registers configuration CH552
    * Fosc = 24000000 Hz
    * Fpwm = Fosc / 256 / PWM_CK_SE
    * Fpwm = 31.250 Hz (Requested : 38000 Hz)
    * Duty Cycle = 50 %
    * 
    * When using the hardware pwm module it seems, that
    * it is not possible to set the PWM carrier freq to
    * 38KHz, therefore this can be achieved using a timer,
    * and a GPIO pin, that way we can be more flexible setting
    * the correct PWM frequency
    * 
    * We have two options, one is the hardware PWM module,
    * the second is the implemented soft PWM, using timer 1.
    */ 
    //setup for IR RX
  #ifndef SOFT_PWM   
  PWM_CK_SE = 3;
  PIN_output(PIN_PWM); 
  // Bring the pwm pin off/IR LED off
  PIN_low(PIN_PWM); 
  // Setup the PWM Duty cycle to 50%
  PWM_write(PIN_PWM, PWM_DUTY_50);  
  #endif
  // configure the IRRX pin as input, pulled up (this is INT0 pin)
  PIN_input_PU(IRRX); 
  // Setup the timer0 Gated by Int0, when int0 becomes high, timer is started
  TMOD |= bT0_M0 | bT1_M0;   /* Run in time mode not counting */ 
  T2MOD = bT1_CLK; /* Divide the system clock by 12 */
  EA  = 1;     /* Enable global interrupt */
  EX0 = 1;    // Enable INT0
  /** INT0 is edge triggered */
  IT0 = 1;    // INT0 is edge triggered
  // Configure Timer 2 in Capture mode. FSYS/12 (2MHz)
  ConfigTimer2();
  // Just in case
  CDC_readPointer = 0;
  // Zero target frequency on startup
  target_freq = 0; 
  // Main loop
  while(1) {
    switch (mode)
    {
      case IR_S:
        if (irsService() != 0) SetUpDefaultMainMode();
        break;
      case IR_MAIN:
        if(CDC_available()) {  // something coming in?
          char byte = CDC_read_b(); // read the character ...  
          switch (byte)
          {
              case 'S': //Sampling Mode IR TX and IR RX
              case 's': 
                  irsSetup();
                  mode = IR_S;
                  break;
              case 'V':
              case 'v':// Acquire Version
                  GetUsbIrdroidVersion();
                  break;
              default:
                break;         
        }
        break;
      }
      default:
        break;
    }   
  }
}
