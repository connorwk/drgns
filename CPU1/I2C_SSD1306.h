/*
 * I2C_SSD1306.h
 *
 *  Created on: Jan 14, 2019
 *      Author: win7 vm
 */

#ifndef I2C_SSD1306_H_
#define I2C_SSD1306_H_

extern void I2C_Init(void);
extern Uint16 I2C_WriteData(Uint16 datacommand, Uint16 message[16], Uint16 length);
extern void I2C_Display_Init(void);
//extern Uint16 MsgBuffer[15];
//extern Uint16 MsgStatus;
extern const unsigned char I2C_Display_Font[96][5];
extern void I2C_Display_Char(char character);
extern void I2C_Display_String(char* string, Uint16 x, Uint16 y);

#endif /* I2C_SSD1306_H_ */
