#pragma once

#ifdef DEBUG
#include "src/oled_term.h" // for OLED Debug
#include "stdio.h"
extern __xdata uint8_t dbg_buff[20];
#define DBG(fmt, ...) sprintf(dbg_buff, "%s: " fmt "\r\n", __func__, ##__VA_ARGS__); OLED_print(dbg_buff);
#endif
#ifndef DEBUG
#define DBG(fmt, ...);
#endif