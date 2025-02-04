#pragma once

#ifdef DEBUG
#include "src/oled_term.h" // for OLED Debug
#include "stdio.h"
// Put the debug buffer in the xRAM section
__xdata uint8_t buffer[20];
#define DBG(fmt, ...) sprintf(buffer, "%s: " fmt "\r\n", __func__, ##__VA_ARGS__); OLED_print(buffer);
#else 
#define DBG(fmt, ...);
#endif