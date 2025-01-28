#ifndef IRS_H
#define IRS_H
// ============================================================================
// Libraries, Definitions and Macros
// ============================================================================
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "src/pwm.h"
#include "src/gpio.h"

#define KB_PLUS 43
#define KB_MINUS 45
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

// ============================================================================
// Function Declarations
// ============================================================================
/** @brief Send the USB Infrared Tranceiver version information to the host */
void GetUsbIrdroidVersion(void);
/** @brief Setup for IR sampling mode; Timer, PWM etc. */
void irsSetup(void);
#endif