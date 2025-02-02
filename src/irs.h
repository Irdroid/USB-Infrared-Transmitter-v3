#ifndef IRS_H
#define IRS_H
// ============================================================================
// Libraries, Definitions and Macros
// ============================================================================
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "src/pwm.h"                      // for PWM Code
#include "src/gpio.h"                     // for GPIO
#include "src/hardwareprofile.h"          // Definitions hw specific

/** Constant that we use to multiply the values coming from th USB host to 
 * match the irtoy time unit*/
#define TIMER_0_CONST 128
#define PWM_DUTY_50 128 // PWM Duty cycle constant for 50% Duty cycle
#define LED_PIN P35 // Macro for the LED PIN
#define PWMon() PWM_start(PIN_PWM); // Macro to turn on the PWM
#define PWMoff() PWM_stop(PIN_PWM); // Macro to turn off the PWM

#define LedOn() PIN_high(LED_PIN); // Turn On the Blue LED
#define LedOff() PIN_low(LED_PIN); // Turn On the Blue LED
#define LedToggle() PIN_toggle(LED_PIN); // Toggle the Blue LED

//
// Irdroid/Irtoy COMMANDS
//
#define IRIO_RESET 		        0x00
#define IRIO_FREQ 		        0x04
#define IRIO_SETUP_SAMPLETIMER  0x05
#define IRIO_SETUP_PWM          0x06
#define IRIO_LEDMUTEON          0x10
#define IRIO_LEDMUTEOFF         0x11
#define IRIO_LEDON 		        0x12
#define IRIO_LEDOFF             0x13
#define IRIO_DESCRIPTOR         0x23
#define IRIO_RETURNTXCNT        0x24
#define IRIO_NOTIFYONCOMPLETE   0X25
#define IRIO_HANDSHAKE          0x26
#define IRIO_TRANSMIT_unit      0x03
#define IRIO_IO_WRITE           0x30
#define IRIO_IO_DIR		        0x31
#define IRIO_IO_READ            0x32
#define IRIO_GETCNT             0x33
#define IRIO_UART_SETUP 	    0x40
#define IRIO_UART_CLOSE		    0x41
#define IRIO_UART_WRITE		    0x42
#define IRIO_IRW_FREQ           0x43
#define CDC_DESC                0x22

typedef enum { //in out data state machine
    I_IDLE = 0,
    I_PARAMETERS,
    I_PROCESS,
    I_DATA_L,
    I_DATA_H,
    I_TX_STATE,
    I_LAST_PACKET //JTR3 New! For 0x07 command
} _smio;

// ============================================================================
// Function Declarations
// ============================================================================

/** @brief Send the USB Infrared Tranceiver version information to the host */
void GetUsbIrdroidVersion(void);

/** @brief Setup for IR sampling mode; Timer, PWM etc. */
void irsSetup(void);

/** @brief Ir service routine */
unsigned char irsService(void);

/** @brief Copy the data from the CDC Out buffer to another buffer in 
 * the memory
 * 
 * @param[in] len - The number of bytes to copy
 * @param[out] buffer - Reference to the buffer to store the data
 * @return the number of bytes copied to the buffer
 */
unsigned char getUnsignedCharArrayUsbUart(uint8_t *buffer, uint8_t len);

/** @brief Timer0 Interrupt callback routine */
void timer0_int_callback(void);  
#endif