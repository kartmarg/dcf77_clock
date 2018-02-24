/* 
 * Functions to drive the Dragon12 LCD module. 
 *
 * Based on the Dragon12 documentation, Lcd.asm example
 * by Wayne Chu, and an implementation by Louis Bertrand,
 * Keith Baldwin, and Justin Ryan from Durham College, Oshawa ON
 *  
 * LCD Module is being used in 4-bit mode.
 * Port K bit assignment:
 *   7 = unused
 *   6 = unused
 *   5 = Module DB7
 *   4 = Module DB6
 *   3 = Module DB5
 *   2 = Module DB4
 *   1 = EN (pulse 1 to write)
 *   0 = RS (0=command, 1=data)
 *
 * $Id:$
 * $Rev:$
 *
 * License: Gnu Lesser Public License (LGPL)
 *
 */

#ifndef _LCDDRV_H_
#define _LCDDRV_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Initialize the LCD module. Must be called before
 * the LCD can be used.
 *
 */
void initLcd(void);


/* 
 * Write a line to the LCD.
 * Inputs:
 *   string: is a pointer to a null terminated array of char to be sent.
 *   line: determines which line to display (0=top line, 1=bottom line).
 * If the string is less than 16 characters long the rest of the line
 * is filled with blanks.
 *
 * Strings longer than 16 characters (excluding the terminating \0) 
 * get truncated. 
 *
 */
void writeLine(char *string, int line);

#ifdef __cplusplus
}
#endif

#endif
