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
#include "stdbool.h"
#include "system.h"
#include "delay.h"
uint8_t count = 0;
uint16_t ticks = 0;
/** The CDC EP2 read pointer */
extern volatile __bit CDC_EP2_readPointer;
/** The CDC EP2 write pointer */
extern volatile __xdata uint8_t CDC_writePointer;
/** IR Receive flag */
__xdata bool rxflag1 = false;
__xdata bool rxflag = false;
bool first = false;

/** References to the CDC in and out buffers */
extern uint8_t * cdc_Out_buffer; 
extern uint8_t * cdc_In_buffer; 

uint8_t *OutPtr; // Same naming of this pointer as in the irtoy code

static unsigned char TxBuffCtr; // Transmit buffer counter
static unsigned char h, l, tmr0_buf[3]; // Timer0 buffer
__xdata struct _irtoy irToy; // We store the irToy structure in the xRAM
// here we save the pulse-space values
// of the captured IR signal
__xdata uint16_t pulse;
__xdata uint16_t space;
__xdata uint16_t pulse1;
__xdata uint16_t space1;
// which buffer we are dealing with
bool inWhichBuf = false;

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
static void whichBuff(void){
  if(!inWhichBuf){
    cdc_In_buffer = (uint8_t *) EP2_buffer+128;
    inWhichBuf = true;
  }else{
    cdc_In_buffer = (uint8_t *) EP2_buffer+192;
    inWhichBuf = false;
  }
}
static void fast_usb_handler(void) {
  // USB transfer completed interrupt
  if(UIF_TRANSFER) {
    // Dispatch to service functions
    uint8_t callIndex = USB_INT_ST & MASK_UIS_ENDP;
    switch (USB_INT_ST & MASK_UIS_TOKEN) {
      case UIS_TOKEN_IN:
        switch (callIndex) {
          #ifdef EP2_IN_callback
          case 2: EP2_IN_callback(); break;
          #endif
          default: break;
        }
        break;
      case UIS_TOKEN_OUT:
        switch (callIndex) {
          #ifdef EP2_OUT_callback
          case 2: EP2_OUT_callback(); break;
          #endif
          default: break;
        }
        break;
    }
    UIF_TRANSFER = 0;                       // clear interrupt flag
  }
}
/** @brief Timer0 Interrupt callback routine */
void timer0_int_callback(void)   
{ 
    //TR0 = 0; // Disable the timer
    //TF0 = 0; // Clear Timer0 interrupt flag
      
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

        }else{
         TR0 = 0;
         TR2 = 0;
         // Put it into the CDC buffer
         //cdc_In_buffer = inWhich();
          //TR0 = 0;
  // Stop timer2
  //TR2 = 0;
  // Read Timer2 readings and substact space to get the pulse value
  pulse = ((TH2 << 8) | TL2);
  pulse = _divuint(pulse, TIMER_0_CONST);
  // Put it into the CDC buffer
  *cdc_In_buffer++ = (pulse >> 8) & 0xff;
  *cdc_In_buffer++ = pulse;
  // Increment the write pointer by 2 octets
  CDC_writePointer += sizeof(uint16_t);
            *cdc_In_buffer++ = 0xff;
            *cdc_In_buffer++ = 0xff;
            
         //   // Increment the write pointer by 2 octets
            CDC_writePointer += 2;
            WaitInReady();
            CDC_flush(); // flush the buffer
            cdc_In_buffer = inWhich();
         //   whichBuff();
          //  count = 0;
           
        }  
}

/** @brief Timer1 Interrupt callback routine */
void timer1_int_callback(void){
    
    if(irS.TX == 0){
       
        irS.flushflag = 1;
        TH1 = 0;
        TL1 = 0;
        T1 = 0;
    }else{
    #ifdef SOFT_PWM
    PIN_toggle(PIN_PWM); // Toggle the PWM pin
     // Set timer1 High and Low SFRs again
    TH1 = (*timer1_pwm_ptr >> 8) & 0xff;
    TL1 = *timer1_pwm_ptr;
    #endif
    // TODO: Check if anything else is needed here
    }
} 

void ext0_interrupt(void) __interrupt(INT_NO_INT0){
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

      TH1 = 0;
      TL1 = 0;
      ET1 = 1;
      TR1 = 1;
     
            
    }else{
      // Stop timer0
      //TR0 = 0;
      // Stop timer2
      //TR2 = 0;
      //reset timer1, USB packet send timeout
      TR1 = 0; //timer off
      TH1 = 0;
      TL1 = 0;
      ET1 = 1;
      TR1 = 1; //timer on
      if(!rxflag1){
        
        // Read Timer0 readings , this is the IR signal space
        space = (TH0 << 8) | TL0;
        // Read Timer2 readings and substact space to get the pulse value
        pulse = ((TH2 << 8) | TL2) - space;
        rxflag1 = true;
      }else{
        // overflow
        DBG("ovf\n");
      }
      // Now we have to zero TH0, TL0, TH2 and TL2
      TH0 = 0;
      TL0 = 0;
      TH2 = 0;
      TL2 = 0;
      // And now we have to write TR0 = 1 and TR2 = 1;
      // Eg. preparing for the next pulse-space cycle
      // Timer0 will start after the INT0 is raised,
      // Timer 2 will start immediatly
      //TR0=1;
      //TR2=1;
    }
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

/** @brief Calculate the IR Tx Carrier frequency in HZ, coming from the host.
 * When the host sends a 0x06 command, e.g. set PWM frequency, the next octet,
 * after the command is the actual PWM setting, we get this value and then use 
 * the PwmConfigure function to set PWM period as appropriate.
 * 
 * @param[in] pwm_setting - the PWM setting coming from the host
 */
static inline uint16_t irtoy_pwm_to_hz(uint8_t pwm_setting){
    return (uint16_t)((IRTOY_FREQ/(pwm_setting+1))/IRTOY_MULTIPLIER);
}

void PwmConfigure(uint16_t freq, uint16_t *timer1_pwm_val){
    //calculate the timer value that we need to set
    // timer1_pwm_val (half period duration) = (1/freq / 1/Timer_clock) / 2
    float target_period = (((1/((float)(freq)))/(SOFT_PWM_MIN_PER))/2.0F);
    *timer1_pwm_val = (uint16_t)(target_period-SPWM_DRIFT);
    // Invert the value which will later be set to timer 1 TH/TL regs
    *timer1_pwm_val = ~(*timer1_pwm_val);
    // Set timer1 High and Low SFRs
    TH1 = (*timer1_pwm_val >> 8) & 0xff;
    TL1 = *timer1_pwm_val;
    ET1 = 1; // Enable Timer 1 interrupt
    // Configure the PWM pin as output
    PIN_output(PIN_PWM);
}

unsigned char getUnsignedCharArrayUsbUart(uint8_t *buffer, uint8_t len){
   
   if(CDC_available())
    {    
       // DBG("count %d\n", CDC_readByteCount);
        if(len > CDC_readByteCount)
        {
            len = CDC_readByteCount;
        }
       if(len != 0){ 
        for (int i=0; i < len; i++){
             buffer[i] = CDC_read_b();
        }
       }
    return len;
    }
   return 0;
}

void GetUsbIrdroidVersion(void) {
    cdc_In_buffer = inWhich();
    cdc_In_buffer[0] = 'V'; //answer OK
    cdc_In_buffer[1] = HARDWARE_VERSION;
    cdc_In_buffer[2] = FIRMWARE_VERSION_H;
    cdc_In_buffer[3] = FIRMWARE_VERSION_L;
    WaitInReady();
    CDC_writePointer += sizeof(uint32_t); // Increment the write counter
    CDC_flush(); // flush the buffer 
}

void irsSetup(void) {
    irS.rxflag1 = 0;
    inWhichBuf = true;
    first = true;
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
    WaitInReady();
    cdc_In_buffer = inWhich();
    cdc_In_buffer[0] = 'S'; //answer to the host that we are in sampling mode
    cdc_In_buffer[1] = '0';
    cdc_In_buffer[2] = '1';
    CDC_writePointer += 3;
    CDC_flush(); 
    //ET1 = 1;
    cdc_In_buffer = inWhich();
     if(cdc_In_buffer == ((uint8_t *) EP2_buffer+128)){
                    inWhichBuf = true;
                }else{
                    inWhichBuf = false;
                }
    whichBuff();
   
}

unsigned char irsService(void)
{   
    static _smio irIOstate = I_IDLE;
    static unsigned int txcnt = 0;
    static unsigned char i = 0;
    if(first){
        CDC_flush();
        first = false;
    }
    if (irS.TXsamples == 0) {
        irS.TXsamples = getUnsignedCharArrayUsbUart(irToy.s, MAX_PACKET_SIZE);
        TxBuffCtr = 0;
    }

    if (irS.TXsamples > 0) {
        
        switch (irIOstate) { 
            case I_IDLE:
                switch (irToy.s[TxBuffCtr]) {
                    case IRIO_TRANSMIT_unit: //start transmitting
                        txcnt = 0; //reset transmit byte counter, used for diagnostic
                        ET0 = 0; // Disable Timer 0 interrupt
                        TR0 = 0; //enable the timer
                        irS.TX = 0;
                        
                        irS.txflag = 0; //transmit flag =0 reset the transmit flag
                        irIOstate = I_TX_STATE; //change to transmit data processing state
						tmr0_buf[2]=0x00; //last data packet flag
						irS.txerror=0; //reset error message
                        LedOff();
                        IE_USB = 0;
                        if (irS.handshake) {
                            cdc_In_buffer = inWhich();
                            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES)| UEP_R_RES_ACK; 
                            WaitInReady();
                            cdc_In_buffer[0] = MAX_PACKET_SIZE;
                            CDC_writePointer += sizeof(uint8_t); // Increment the write counter
                            CDC_flush(); // flush the buffer 
                        }      
                        
                        do {
                            
                            irS.TXsamples = getCDC_Out_ArmNext();
                            OutPtr = OutWhich();   
                            if (irS.TXsamples) { // host may have sent a ZLP skip transmit if so.
                                       
                                        // Ask for more bytes
                                        UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES)| UEP_R_RES_ACK;  
                                        // Ask the host to send us 62 bytes
                                        if (irS.handshake) {
                                            cdc_In_buffer = inWhich();
                                            WaitInReady();
                                            cdc_In_buffer[0] = MAX_PACKET_SIZE;
                                            CDC_writePointer += sizeof(uint8_t); // Increment the write counter
                                            CDC_flush(); // flush the buffer 
                                            fast_usb_handler(); 
                                            fast_usb_handler(); 
                                        }                  
                                    
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
                                        *(OutPtr) = 40;
                                        // Check if this need to be here
                                        CDC_readByteCount = 0;
                                    }

                                    align_irtoy_ch552(*OutPtr, *(OutPtr+1), OutPtr);

                                    // This cute code calculates the two's compliment (subtract from zero)
                                    // The quick way to do this in invert and add 1.
                                    *OutPtr = ~*OutPtr;
                                    *(OutPtr + 1) = ~*(OutPtr + 1);

                                    *(OutPtr + 1) += 1;
                                    if (*(OutPtr + 1) == 0) // did we get rollover in LSB?
                                        *(OutPtr) += 1; // then must add the carry to MSB

                                    while (irS.txflag == 1){
                                        fast_usb_handler(); 
                                    }
                                   
                                    tmr0_buf[1] = *(OutPtr); //put the second byte in the buffer
                                    tmr0_buf[0] = *(OutPtr + 1); //put the first byte in the buffer
                                    
                                    txcnt += 2; //total bytes transmitted
                                    // Decrement the number of bytes read
                                    //CDC_readByteCount -= 2;

                                    if (irS.TX == 0) {//enable interrupt if this is the first time
                                        fast_usb_handler(); 
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
                        IE_USB = 1;
                        if (irS.sendcount) { //return the total number of bytes transmitted if required
                            WaitInReady();
                            cdc_In_buffer = inWhich();
                            cdc_In_buffer[0] = 't';
                            cdc_In_buffer[1] = (txcnt >> 8)&0xff;
                            cdc_In_buffer[2] = (txcnt & 0xff);
                            CDC_writePointer += 3;
                            CDC_flush(); // flush the buffer 
                        }
                        while (irS.txflag == 1){
                            fast_usb_handler(); 
                        }
                        LedOff();
                        if (irS.sendfinish) { // Really redundant giving we can send a count above.
                            WaitInReady();
                            cdc_In_buffer = inWhich();
                            cdc_In_buffer[0] = 'C';
							if(irS.txerror) cdc_In_buffer[0] = 'F';
                            CDC_writePointer += 1;
                            CDC_flush(); // flush the buffer 
                            //USB_interrupt();
                        }
                        
                        irS.TXsamples = 1; //will be zeroed at end, super hack yuck!  //JTR3 super yuck!
                         cdc_In_buffer = inWhich();
                if(cdc_In_buffer == ((uint8_t *) EP2_buffer+128)){
                    inWhichBuf = false;
                }else{
                    inWhichBuf = true;
                }
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
                        cdc_In_buffer = inWhich();
                        cdc_In_buffer[0] = 't';
                        cdc_In_buffer[1] = (txcnt >> 8)&0xff;
                        cdc_In_buffer[2] = (txcnt & 0xff);
                        CDC_writePointer += 3;
                        CDC_flush(); // flush the buffer 
                        break;
                    case IRIO_RETURNTXCNT:
                        irS.sendcount = 1;
                        break;
                    case IRIO_SETUP_PWM:
                        TxBuffCtr++;
						/* Check if we have a command to jump to the bootloader */
                        if(irToy.s[TxBuffCtr] == 0xff){
                        	DBG("BOOT");
                        	WDT_start();
                        	while(1); 
                        }else{
							/** Convert Irtoy PWM setting to HZ */
							uint16_t freq = irtoy_pwm_to_hz(irToy.s[TxBuffCtr]);
							DBG("Frequency(Hz): %u\n", freq)
							/* Configure the software PWM setting for the desired frequency */
							PwmConfigure(freq, timer1_pwm_ptr);
						}
                        irS.TXsamples -=2;
                        break;
                    case CUSTOM_FF:
                        LedOff();
                        DBG("IR Reset %x\n", irToy.s[TxBuffCtr]);
                        return 1; //need to flag exit!
                    default:
                        break;
                }
                irS.TXsamples--;
                TxBuffCtr++;
               
            break;
        }   
    
    }
    // If we have pulse-space measuremnts available, put them in the CDC buffer
    if(rxflag1){
    rxflag1 = false;
      // Divide by this constant to achieve irtoy time unit
      // and report measurements to the host as if it is the irtoy hardware 
      pulse = _divuint(pulse, TIMER_0_CONST);
      space = _divuint(space, TIMER_0_CONST);
      *cdc_In_buffer++ = (pulse >> 8) & 0xff;
      *cdc_In_buffer++ = pulse;
      *cdc_In_buffer++ = (space >> 8) & 0xff;
      *cdc_In_buffer++ = space;
      CDC_writePointer += sizeof(uint32_t);
      
    }
    if(CDC_writePointer == MAX_PACKET_SIZE){
        WaitInReady();
        CDC_flush(); // flush the buffer
        while(CDC_writeBusyFlag);
        cdc_In_buffer = inWhich();
    }
    if(irS.flushflag == 1){
        irS.flushflag = 0;
        ET1 = 0;
        WaitInReady();
        CDC_flush(); // flush the buffer
        while(CDC_writeBusyFlag);
        cdc_In_buffer = inWhich();
    }
    return 0;
}