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
// This is a port of the Irdroid USB Infrared Tranceiver firmware to 
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
#include "src/oled_term.h"                // for OLED
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "src/pwm.h"

#define KB_PLUS 43
#define KB_MINUS 45
// Constant that we use to multiply the values coming from th USB host to match the irtoy time unit
#define TIMER_0_CONST 128
#define PWM_DUTY_50 128 // PWM Duty cycle constant for 50% Duty cycle
#define LED_PIN P35 // Macro for the LED PIN
#define PWMon() PWM_start(PIN_PWM); // Macro to turn on the PWM
#define PWMoff() PWM_stop(PIN_PWM); // Macro to turn off the PWM

#define LedOn() PIN_high(LED_PIN); // Turn On the Blue LED
#define LedOff() PIN_low(LED_PIN); // Turn On the Blue LED
#define LedToggle() PIN_toggle(LED_PIN); // Toggle the Blue LED

extern volatile __bit CDC_EP2_readPointer;
uint8_t buf[2];
uint8_t * cdc_Out_Buffer = (uint8_t *) EP2_buffer; 
uint8_t * cdc_In_Buffer = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 

void timer0_interrupt(void) __interrupt(INT_NO_TMR0)   
{
    
   
    PIN_toggle(PIN_BUZZER);
   
   
    TH0 = buf[0];     
    TL0 = buf[1];      
}

static inline void align_irtoy_ch552(uint8_t timer_h, uint8_t timer_l, uint8_t *buf){
    register uint16_t time_val = ((timer_h << 8) | timer_l);
    time_val = time_val*TIMER_0_CONST;
    *buf++ = (time_val >> 8) & 0xff;
    *buf = time_val;
}

/** @brief A Structure, holdingextern uint8_t  Irdroid USB Infrared Transceiver IRs data */
static struct {
    unsigned char RXsamples;
    unsigned char TXsamples;
    unsigned char timeout;
    unsigned char TX : 1;
    unsigned char rxflag1 : 1;
    unsigned char rxflag2 : 1;
    unsigned char txflag : 1;
    unsigned char flushflag : 1;
    unsigned char overflow : 1;
    unsigned char TXInvert : 1;
    unsigned char handshake : 1;
    unsigned char sendcount : 1;
    unsigned char sendfinish : 1;
    unsigned char RXcompleted : 1;
	unsigned char txerror : 1;
} irS;
extern volatile __xdata uint8_t CDC_writePointer;

void GetUsbIrToyVersion(void) {
        
        cdc_In_Buffer[0] = 'V'; //answer OK
        cdc_In_Buffer[1] = '2';
        cdc_In_Buffer[2] = '2';
        cdc_In_Buffer[3] = '5';
        WaitInReady();
        CDC_writePointer += sizeof(uint32_t); // Increment the write counter
        CDC_flush(); // flush the buffer 
}

void irsSetup(uint8_t * buf) {
 
       /*
     * PWM registers configuration CH552
     * Fosc = 24000000 Hz
     * Fpwm = 37.538 Hz (Requested : 38000 Hz)
     * Duty Cycle = 50 %
     */
    //PWM_set_freq(14000);                    
    PWM_CK_SE = 5;
    PIN_output(PIN_PWM); 
    // Setup the PWM Duty cycle to 50%
    PWM_write(PIN_PWM, PWM_DUTY_50);  
    
    // Setup Timer0 & enable the interrupts
    EA  = 1;            /* Enable global interrupt */
    ET0 = 1;            /* Enable timer0 interrupt */

 

    TMOD = 0x1;   /* Run in time mode not counting */ 
    T2MOD =0b00010000; /* Divide the system clock by 4 */

    
}
// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

// ===================================================================================
// Buzzer Function
// ===================================================================================

// Create a short beep on the buzzer
void beep(void) {
  uint8_t i;
  for(i=255; i; i--) {
    PIN_low(PIN_BUZZER);
    DLY_us(125);
    PIN_high(PIN_BUZZER);
    DLY_us(125);
  }
}

// ===================================================================================
// Main Function
// ===================================================================================

void main(void) {
 
  // Setup
  CLK_config();                           // configure system clock
  DLY_ms(5);                              // wait for clock to stabilize
  CDC_init();                             // init USB CDC
  OLED_init();                            // init OLED
  irsSetup(buf);
  // Print start message
  OLED_print("* CDC OLED TERMINAL *");
  OLED_print("---------------------");
  OLED_print("Ready\n");
  OLED_print("_\r");
  beep();
  // buffer to store the data coming from the host alternatevly use the USB CDC buffer
  buf[0] = 0x1;
  buf[1] = 0x2C;
  // Take into consideration the differences for minimal time unit between Irdroid and CH552 timers
  align_irtoy_ch552(buf[0], buf[1], buf);

  uint8_t *OutPtr = buf;
  
  *OutPtr = ~*OutPtr;
  *(OutPtr + 1) = ~*(OutPtr + 1);

  *(OutPtr + 1) += 1;
  if (*(OutPtr + 1) == 0) // did we get rollover in LSB?
  *(OutPtr) += 1; // then must add the carry to MSB

  TH0 = buf[0];     
  TL0 = buf[1];

  TR0 = 1;            /* Start timer0 */

  // Loop
  while(1) {
    if(CDC_available()) {                 // something coming in?
      char c = CDC_read();                // read the character ...
      OLED_write(c);                      // ... and print it on the OLED
      if((c == 10) || (c == 7)) beep();   // beep on newline command
      switch (c)
      {
      case KB_PLUS:
        PWMon();
        break;
      
      case KB_MINUS:
        GetUsbIrToyVersion();
        PWMoff();
        break;

      case 'v':
        GetUsbIrToyVersion();
        break;
      }
    }
  }
}
