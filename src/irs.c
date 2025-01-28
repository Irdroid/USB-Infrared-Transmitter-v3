// ===================================================================================
// Project:   Irdroid USB Infrared Transceiver for CH551, CH552, CH554
// Version:   v1.0
// Year:      2025
// Author:    Georgi Bakalski
// Github:    https://github.com/irdroid
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// File:      irs.c Samling routines
// ===================================================================================
#include "src/irs.h"
/** The CDC EP2 read pointer */
extern volatile __bit CDC_EP2_readPointer;
/** The CDC EP2 write pointer */
extern volatile __xdata uint8_t CDC_writePointer;

/** References to the CDC in and out buffers */
uint8_t * cdc_Out_Buffer = (uint8_t *) EP2_buffer; 
uint8_t * cdc_In_Buffer = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 

static unsigned char TxBuffCtr;
__xdata struct _irtoy irToy;

#define IRS_TRANSMIT_HI	0
#define IRS_TRANSMIT_LO	1

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

/** @brief Timer0 Interrupt routine */
void timer0_interrupt(void) __interrupt(INT_NO_TMR0)   
{ 
    TH0 = 0;     
    TL0 = 1;      
}
// ============================================================================
// Function Definitions
// ============================================================================

/** @brief Align the irtoy time to the ch552 time unit */
static inline void align_irtoy_ch552(uint8_t timer_h, uint8_t timer_l, uint8_t *buf){
    register uint16_t time_val = ((timer_h << 8) | timer_l);
    time_val = time_val*TIMER_0_CONST;
    *buf++ = (time_val >> 8) & 0xff;
    *buf = time_val;
}

uint8_t getUnsignedCharArrayUsbUart(uint8_t *buffer, uint8_t len){
    
    if(CDC_available()){
        
        if(len > CDC_readByteCount){
            len = CDC_readByteCount;
        }
        
        for (CDC_readByteCount; CDC_readByteCount < len; CDC_readByteCount++){
             buffer[len] = cdc_Out_Buffer[CDC_readByteCount];
        }
               

    }
    return CDC_readByteCount;
}

void GetUsbIrdroidVersion(void) {
        cdc_In_Buffer[0] = 'V'; //answer OK
        cdc_In_Buffer[1] = '2';
        cdc_In_Buffer[2] = '2';
        cdc_In_Buffer[3] = '5';
        WaitInReady();
        CDC_writePointer += sizeof(uint32_t); // Increment the write counter
        CDC_flush(); // flush the buffer 
}

void irsSetup(void) {
     
    cdc_In_Buffer[0] = 'S'; //answer that we are in sampling mode
    cdc_In_Buffer[1] = '0';
    cdc_In_Buffer[2] = '1';
    WaitInReady();
    CDC_writePointer += 3;
    CDC_flush();
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

unsigned char irsService(void)
{
    if (irS.TXsamples == 0) {
        irS.TXsamples = getUnsignedCharArrayUsbUart(irToy.s, MAX_PACKET_SIZE);
        TxBuffCtr = 0;
    }
    
    WaitInReady();

    //if (irS.TXsamples > 0) {
    //}

    return 0;
}