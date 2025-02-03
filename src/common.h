#pragma once

#include "config.h"
/** Simple debugging via a connected OLED LCD Screen.
 * There is no information about any other possible option (maybe uart).
 * For now the debugging via the i2c connected OLED screen does the job.
 */
#ifdef DEBUG
#include "stdio.h"
__xdata uint8_t buffer[20];
#define DBG(fmt, ...) sprintf(buffer, "%s: " fmt "\r\n", __func__, ##__VA_ARGS__); OLED_print(buffer);
#else 
#define DBG(fmt, ...);
#endif