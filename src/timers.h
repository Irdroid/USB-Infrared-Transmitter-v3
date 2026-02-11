// ===================================================================================
// Timer helper functions for the Irdroid USB Infrared Transceiver v3 firmware.
// ===================================================================================
// Simple helper functions, used to initialize Timer1 and Timer2
// 
// Description: This is a port of the Irdroid USB Infrared Tranceiver firmware from 
// PIC18F2550 to CH551, CH552, CH554
//
// Author: Georgi Bakalski JAN 2026
//
// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================
#pragma once
#include <stdint.h>
// ===================================================================================
// Definitions and Macros
// ===================================================================================
#define T1_CLK_DIV12  0     // Divide system clock by 12
#define T1_CLK_DIV1   1     // Use the system clock w/o division
#define ENABLE_TIMER2() ET2=1;TR2=1;    // Enable Timer2
#define DISABLE_TIMER2() ET2 = 0;TR2=0; // Disable Timer 2

// ===================================================================================
// Function declarations
// ===================================================================================

/**
 * @brief Configures Timer 1 Clock
 * @param clock: T1_CLK_DIV12 (Standard) or T1_CLK_FSYS (Fast)
 */
void ConfigTimer1(uint8_t clock);

/** @brief Configures Timer 2 in Capture mode.
 *  The timer value is captured on each Edge of the T2EX pin
 *  pin 1.1, and triggers from any -to- any edge of the 
 *  signal. Timer 2 is started separately using interrupt
 *  from the INT0 pin, wired also to the IR Receiver, together
 *  with the T2EX pin. In this way we can measure the signal
 *  timings from the start.
 */
void ConfigTimer2(void);

/** @brief We use Timer 1, to send any pending data
 *  to the USB host after approximately 2.7ms.
 *  The timer is restarted on each edge of the T2EX pin.
 */
void RestartTimer1(void);