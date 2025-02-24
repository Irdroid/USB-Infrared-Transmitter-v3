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
#include "common.h"

/** The CDC EP2 read pointer */
extern volatile __bit CDC_EP2_readPointer;
/** The CDC EP2 write pointer */
extern volatile __xdata uint8_t CDC_writePointer;

/** References to the CDC in and out buffers */
uint8_t * cdc_Out_buffer = (uint8_t *) EP2_buffer; 
uint8_t * cdc_In_buffer = (uint8_t *) EP2_buffer+MAX_PACKET_SIZE; 

uint8_t *OutPtr; // Same naming of this pointer as in the irtoy code

static unsigned char TxBuffCtr; // Transmit buffer counter
static unsigned char h, l, tmr0_buf[3]; // Timer0 buffer
__xdata struct _irtoy irToy; // We store the irToy structure in the xRAM

#define IRS_TRANSMIT_HI	0
#define IRS_TRANSMIT_LO	1

#ifdef SOFT_PWM
__xdata uint16_t timer1_pwm_val;
__xdata uint16_t *timer1_pwm_ptr = &timer1_pwm_val;
#endif

/** @brief A Structure, holding the Irdroid USB Infrared Transceiver IRs data */
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

/** @brief Timer0 Interrupt callback routine */
void timer0_int_callback(void)   
{ 
    TR0 = 0; // Disable the timer
    TF0 = 0; // Clear Timer0 interrupt flag
      
      if (irS.TX == 1) {//timer0 interrupt means the IR transmit period is over
            ET0 = 0; // Disable Timer 0 interrupt

			//in transmit mode, but no new data is available
            if (irS.txflag == 0) { 

				if(tmr0_buf[2]==0x00) irS.txerror=1; //if not end flag, raise buffer underrun error
                //disable the PWM, output ground
                PWMoff();
                LedOff();
                irS.TX = 0;
                return;
            }

            if (irS.TXInvert == IRS_TRANSMIT_HI) {
                //enable the PWM
                PWMon();
                irS.TXInvert = IRS_TRANSMIT_LO;
            } else {
                //disable the PWM, output ground
                PWMoff();
                irS.TXInvert = IRS_TRANSMIT_HI;
            }
             //setup timer
            TH0 = tmr0_buf[1]; //first set the high byte
            TL0 = tmr0_buf[0]; //set low byte copies high byte too
    
            TF0 = 0; // Clear the interrupt flag of timer 0
            ET0 = 1; // Enable Timer 0 interrupt
            TR0 = 1; // Enable the timer
            irS.txflag = 0; //buffer ready for new byte

        }  
}

/** @brief Timer1 Interrupt callback routine */
void timer1_int_callback(void){
    
    #ifdef SOFT_PWM
    PIN_toggle(PIN_PWM); // Toggle the PWM pin
     // Set timer1 High and Low SFRs again
    TH1 = (*timer1_pwm_ptr >> 8) & 0xff;
    TL1 = *timer1_pwm_ptr;
    #endif
    // TODO: Check if anything else is needed here
} 
// ============================================================================
// Function Definitions
// ============================================================================

/** @brief Align the irtoy time to the ch552 time unit 
 * The irtoy time unit is with resolution of 21.333us and
 * the CH55x timer0 is configured for 167ns thus to be compatible
 * with the existing Irtoy/Irdroid Driver on the host side, we need 
 * to align that. This function should perform as fast as possible
 * 
 * @param[in] timer_h - The high byte in the buffer,comming from the host
 * @param[in] timer_l - The low bytes in the buffer, comming from the host
 * @param[out] buf - The buffer in which we are storing the result
*/
static inline void align_irtoy_ch552(uint8_t timer_h, uint8_t timer_l, uint8_t *buf){
    uint_fast16_t time_val = ((timer_h << 8) | timer_l);
    time_val = time_val*TIMER_0_CONST;
    *buf++ = (time_val >> 8) & 0xff;
    *buf = time_val;
}

void PwmConfigure(uint16_t freq, uint16_t *timer1_pwm_val){
    //calculate the timer value that we need to set
    // timer1_pwm_val (half period duration) = (1/freq / 1/Timer_clock) / 2
    float target_period = (((1/((float)(freq)))/(SOFT_PWM_MIN_PER))/2.0F);
    *timer1_pwm_val = (uint16_t)(target_period - SPWM_DRIFT);
    // Invert the value which will later be set to
    // TH1 = (timer1_pwm_val >> 8) & 0xff;
    // TL1 = timer1_pwm_val;
    *timer1_pwm_val = ~(*timer1_pwm_val);
    // Set timer1 High and Low SFRs
    TH1 = (*timer1_pwm_val >> 8) & 0xff;
    TL1 = *timer1_pwm_val;
    ET1 = 1; // Enable Timer 1 interrupt
    // Configure the PWM pin as output
    PIN_output(PIN_PWM);
}

unsigned char getUnsignedCharArrayUsbUart(uint8_t *buffer, uint8_t len){
    
    WaitOutReady();
    if(CDC_available())
    {    
        if(len > CDC_readByteCount)
        {
            len = CDC_readByteCount;
        }
        for (int i=0; i < len; i++){
             buffer[i] = CDC_read_b();
        }
    }
    return len;
}

void GetUsbIrdroidVersion(void) {
    cdc_In_buffer[0] = 'V'; //answer OK
    cdc_In_buffer[1] = HARDWARE_VERSION;
    cdc_In_buffer[2] = FIRMWARE_VERSION_H;
    cdc_In_buffer[3] = FIRMWARE_VERSION_L;
    WaitInReady();
    CDC_writePointer += sizeof(uint32_t); // Increment the write counter
    CDC_flush(); // flush the buffer 
}

void irsSetup(void) {
     
    cdc_In_buffer[0] = 'S'; //answer to the host that we are in sampling mode
    cdc_In_buffer[1] = '0';
    cdc_In_buffer[2] = '1';
    WaitInReady();
    CDC_writePointer += 3;
    CDC_flush();
    
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
    
    #ifndef SOFT_PWM   
    PWM_CK_SE = 3;
    PIN_output(PIN_PWM); 
    //PIN_low(PIN_PWM); 
    // Setup the PWM Duty cycle to 50%
    PWM_write(PIN_PWM, PWM_DUTY_50);  
    #else
    // Configure Soft PWM for 38KHz carrier
    PwmConfigure(38000, timer1_pwm_ptr);
    #endif
    PWMon();

    // Setup Timer0 & enable the interrupts
    EA  = 1;            /* Enable global interrupt */
    ET0 = 0;            /* Enable timer0 interrupt */
    TMOD = bT1_M0 | bT0_M0;   /* Run in time mode not counting T0/T1 */ 
    /* By default we are running 24MHz system clock and 2MHz timer clock */
    #ifdef TIMER_CLOCK_FAST
    T2MOD =0b00010000; /* Divide the system clock by 4 */
    #else
    T2MOD =0b00000000; /* Divide the system clock by 12 */
    #endif
     //setup for IR RX
    irS.rxflag1 = 0;
    irS.rxflag2 = 0;
    irS.txflag = 0;
    irS.flushflag = 0;
    irS.timeout = 0;
    irS.RXsamples = 0;
    irS.TXsamples = 0;
    irS.TX = 0;
    irS.overflow = 0;
    irS.sendcount = 0;
    irS.sendfinish = 0;
    irS.handshake = 0;
    irS.RXcompleted = 0;
}

unsigned char irsService(void)
{   
    static _smio irIOstate = I_IDLE;
    static unsigned int txcnt;
    static unsigned char i;
    irS.handshake = 1;

    if (irS.TXsamples == 0) {
        irS.TXsamples = getUnsignedCharArrayUsbUart(irToy.s, MAX_PACKET_SIZE);
        TxBuffCtr = 0;
    }
    
    WaitInReady();

    if (irS.TXsamples > 0) {
        
        switch (irIOstate) {
            
            case I_IDLE:
                    
                switch (irToy.s[TxBuffCtr]) {
                   
                    case IRIO_TRANSMIT_unit: //start transmitting
                        
                        DBG("Transmit mode %x\n", irToy.s[TxBuffCtr]);
                        txcnt = 0; //reset transmit byte counter, used for diagnostic
                        ET0 = 0; // Disable Timer 0 interrupt
                        TR0 = 0; //enable the timer
                        irS.TX = 0;
                        
                        irS.txflag = 0; //transmit flag =0 reset the transmit flag
                        irIOstate = I_TX_STATE; //change to transmit data processing state
						tmr0_buf[2]=0x00; //last data packet flag
						irS.txerror=0; //reset error message
                        irS.handshake = 1;
                        LedOff();
                        
                        if (irS.handshake) {
                            //WaitInReady();
                            cdc_In_buffer[0] = MAX_PACKET_SIZE - 2;
                            CDC_writePointer += sizeof(uint8_t); // Increment the write counter
                            CDC_flush(); // flush the buffer 
                        }      

                        do {
                            OutPtr = cdc_Out_buffer;    
                            WaitOutReady();
                            irS.TXsamples = getCDC_Out_ArmNext();
                            if (irS.TXsamples) { // host may have sent a ZLP skip transmit if so.

                                for (i = 0; i < irS.TXsamples; i += 2, OutPtr += 2) {

                                    // JTR 3 The idea here is to preprocess the "OVERHEAD"
                                    // In what is otherwise dead time. I.E. waiting for the
                                    // IR Tx Timer to timeout.
                                    //check here for 0xff 0xff and return to IDLE state        

                                    if (((*(OutPtr) == 0xff) && (*(OutPtr + 1)) == 0xff)) {
										tmr0_buf[2]=0xff; //flag end of data
                                        irIOstate = I_LAST_PACKET;
                                        i = irS.TXsamples;
                                        *(OutPtr + 1) = 0; // JTR3 replace 0xFFFF with 0020 (Ian's value)
                                        *(OutPtr) = 20;
                                    }

                                    align_irtoy_ch552(*OutPtr, *(OutPtr+1), OutPtr);

                                    // This cute code calculates the two's compliment (subtract from zero)
                                    // The quick way to do this in invert and add 1.
                                    *OutPtr = ~*OutPtr;
                                    *(OutPtr + 1) = ~*(OutPtr + 1);

                                    *(OutPtr + 1) += 1;
                                    if (*(OutPtr + 1) == 0) // did we get rollover in LSB?
                                        *(OutPtr) += 1; // then must add the carry to MSB

                                    while (irS.txflag == 1);
                                   
                                    tmr0_buf[1] = *(OutPtr); //put the second byte in the buffer
                                    tmr0_buf[0] = *(OutPtr + 1); //put the first byte in the buffer
                                    
                                    txcnt += 2; //total bytes transmitted
                                    // Decrement the number of bytes read
                                    CDC_readByteCount -= 2;

                                    if(CDC_readByteCount == 0){
                                        // Ask for more bytes
                                        UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES)| UEP_R_RES_ACK;  
                                        // Ask the host to send us 62 bytes
                                        if (irS.handshake) {
                                            WaitInReady();
                                            cdc_In_buffer[0] = MAX_PACKET_SIZE - 2;
                                            CDC_writePointer += sizeof(uint8_t); // Increment the write counter
                                            CDC_flush(); // flush the buffer 
                                        }                  
                                    }

                                    if (irS.TX == 0) {//enable interrupt if this is the first time

                                        irS.TX = 1;
                                        TH0 = tmr0_buf[1]; //first set the high byte
                                        TL0 = tmr0_buf[0]; //set low byte copies high byte too
                                       
                                        TF0 = 0; // Clear the interrupt flag of timer 0
                                        ET0 = 1; // Enable Timer 0 interrupt
                                        TR0 = 1; //enable the timer
                                        //enable the PWM
                                        PWMon();
                                        irS.TXInvert = IRS_TRANSMIT_LO;
                                        LedOn();
                                    }else{
										//only set AFTER 1st packet or the first packet is sent twice
                                    	irS.txflag = 1; //reset the interrupt buffer full flag
									}

                                    if (irIOstate == I_LAST_PACKET)
                                        break;
                                }//for
                            }
                        } while (irIOstate != I_LAST_PACKET);

                        irIOstate = I_IDLE;
                        
                        if (irS.sendcount) { //return the total number of bytes transmitted if required
                            WaitInReady();
                            cdc_In_buffer[0] = 't';
                            cdc_In_buffer[1] = (txcnt >> 8)&0xff;
                            cdc_In_buffer[2] = (txcnt & 0xff);
                            CDC_writePointer += 3;
                            CDC_flush(); // flush the buffer 
                        }
                        while (irS.txflag == 1);
                        LedOff();
                        if (irS.sendfinish) { // Really redundant giving we can send a count above.
                            WaitInReady();
                            cdc_In_buffer[0] = 'C';
							if(irS.txerror) cdc_In_buffer[0] = 'F';
                            CDC_writePointer += 1;
                            CDC_flush(); // flush the buffer 

                        }
                        irS.TXsamples = 1; //will be zeroed at end, super hack yuck!  //JTR3 super yuck!
                        break;
                    
                    case IRIO_RESET: //reset, return to RC5 (same as SUMP)
                        LedOff();
                        DBG("IR Reset %x\n", irToy.s[TxBuffCtr]);
                        return 1; //need to flag exit!
                        break;

                    case IRIO_FREQ:
                        break;
                    case IRIO_LEDMUTEON:
                        break;
                    case IRIO_LEDMUTEOFF:
                        break;
                    case IRIO_LEDON:
                        LedOn();
                        break;
                    case IRIO_LEDOFF:
                        LedOff();
                        break;
                    case IRIO_HANDSHAKE:
                        DBG("HANDSHAKE %x\n", irToy.s[TxBuffCtr]);
                        irS.handshake = 1;
                        break;
                    case IRIO_NOTIFYONCOMPLETE:
                        DBG("NOTIFY COMPLETE %x\n", irToy.s[TxBuffCtr]);
                        irS.sendfinish = 1;
                        break;
                    case IRIO_GETCNT:
                        WaitInReady();
                        cdc_In_buffer[0] = 't';
                        cdc_In_buffer[1] = (txcnt >> 8)&0xff;
                        cdc_In_buffer[2] = (txcnt & 0xff);
                        CDC_writePointer += 3;
                        CDC_flush(); // flush the buffer 
                        break;
                    case IRIO_RETURNTXCNT:
                        irS.sendcount = 1;
                        break;
                        // JTR3 End of new commands
                    default:
                        break;
                }
                irS.TXsamples--;
                TxBuffCtr++;
            break;
        }   
    
    }

    return 0;
}