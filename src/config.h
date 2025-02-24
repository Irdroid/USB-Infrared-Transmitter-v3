// ===================================================================================
// User configurations
// ===================================================================================

#pragma once

// Pin definitions
#define PIN_BUZZER          P15       // buzzer pin
#define PIN_SDA             P16       // I2C SDA
#define PIN_SCL             P17       // I2C SCL
#define PIN_PWM             P34       // PWM pin
#define SOFT_PWM_MIN_PER    0.0000005F// minimum timer tick for soft PWM
#define SOFT_PWM

#ifdef SOFT_PWM
#define PWM_FREQ            38000     // PWM Carrier is 38KHz
#endif
// USB device descriptor
#define USB_VENDOR_ID       0x4d8    // VID Irdroid
#define USB_PRODUCT_ID      0xf58c   // PID Irdroid 
#define USB_DEVICE_VERSION  0x0300    // v1.0 (BCD-format)

// USB configuration descriptor
#define USB_MAX_POWER_mA    100       // max power in mA 

// USB descriptor strings
#define MANUFACTURER_STR    'H','a','r','d','w','a','r','e',' ','G','r','o','u','p',' ','L','T','D'
#define PRODUCT_STR         'U','S','B',' ','I','n','f','r','a','r','e','d',' ','T','r','a','n','s' \
                            ,'c','e','i','v','e','r'
#define SERIAL_STR          '3','0','0'
#define INTERFACE_STR       'C','D','C',' ','S','e','r','i','a','l'
