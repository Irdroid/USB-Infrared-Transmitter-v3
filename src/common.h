#pragma once

#include "config.h"

#ifdef DEBUG
#include "stdio.h"
__xdata uint8_t buffer[20];
#define DBG(fmt, ...) sprintf(buffer, "%s: " fmt "\r\n", __func__, ##__VA_ARGS__); OLED_print(buffer);
#else 
#define DBG(fmt, ...);
#endif