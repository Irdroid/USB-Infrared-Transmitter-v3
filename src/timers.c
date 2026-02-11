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
#include "timers.h"
#include "ch554.h"
// ===================================================================================
// Function definitions
// ===================================================================================

void ConfigTimer1(uint8_t clock) {
    // Standard Timer 1 16-bit mode initialization
    TMOD &= 0x0F; 
    TMOD |= 0x10;  // Mode 1 (16-bit), C/T=0 (Timer)

    if (clock == T1_CLK_DIV1) {
        T2MOD |= bTMR_CLK;  // Set global TMR_CLK to 1 (no /4 divider)
        T2MOD |= bT1_CLK;   // Set T1_CLK to 1 (enable fast mode)
    } else {
        // Revert to Standard Mode: Fsys/12
        T2MOD &= ~bT1_CLK;  // Clearing bT1_CLK forces Fsys/12 regardless of bTMR_CLK
    }
}
void ConfigTimer2(void) {
    // 2. Configure Timer 2 for Capture Mode
    T2CON = 0x00;           // Clear T2CON
    CP_RL2 = 1;    // CP_RL2 = 1: Capture mode
    EXEN2 = 1;      // EXEN2 = 1: Enable T2EX (P1.1) trigger
    // 3. Set Capture Edge to "Any Edge" (01)
    // Clear both bits first, then set M0 to 1
    T2MOD &= ~bT2_CAP_M1;   // M1 = 0
    T2MOD |= bT2_CAP_M0;    // M0 = 1 -> "Any Edge" mode
}
void RestartTimer1(void)
{
    TR1 = 0; //timer off
    TH1 = 0; //zero the TH1
    TL1 = 0; //zero the TL1
    ET1 = 1; //enable Timer 1 irq
    TR1 = 1; //timer on
}
