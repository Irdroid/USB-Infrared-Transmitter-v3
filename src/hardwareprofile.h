#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#define FIRMWARE_VERSION_H '3'
#define FIRMWARE_VERSION_L '0'

#define SAMPLE_ARRAY_SIZE 0x0040

struct _irtoy {
    unsigned char s[SAMPLE_ARRAY_SIZE];
    unsigned char HardwareVersion;
};
#endif