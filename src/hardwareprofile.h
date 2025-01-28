#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#define FIRMWARE_VERSION_H '2'
#define FIRMWARE_VERSION_L '5'

#define SAMPLE_ARRAY_SIZE 0x0040

struct _irtoy {
    unsigned char s[SAMPLE_ARRAY_SIZE];
    //JTR2 this appears not to be used:
    //unsigned char usbIn[30];
    //unsigned char usbOut[64];
    unsigned char HardwareVersion;
    //  unsigned char CrtlByte;
};

#endif