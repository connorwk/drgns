/*
 * F2837xD_support.h
 * Support file for F2837xD line of chips.
 *  Created on: Jan 29, 2019
 *      Author: Connor Krukosky
 */

#ifndef F2837XD_SUPPORT_H_
#define F2837XD_SUPPORT_H_

#include "F28x_Project.h"
#include "F2837xD_Ipc_drivers.h"
#include "I2C_SSD1306.h"
#include "SCI.h"
#include "stdio.h"
typedef uint_fast8_t uint8_t;
typedef int_fast8_t int8_t;
#define F_CPU 200
#define _delay_us(A)  F28x_usDelay(((((long double) A * 1000.0L) / (long double)CPU_RATE) - 9.0L) / 5.0L)
#define _delay_ms(A)  F28x_usDelay(((((long double) A * 1000000.0L) / (long double)CPU_RATE) - 9.0L) / 5.0L)

#endif /* F2837XD_SUPPORT_H_ */
