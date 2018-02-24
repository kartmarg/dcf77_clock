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

#include "lcddrv.h"
#include <mc9s12dp256.h>

/* Magic delay constants, based on 24MHz CPU clock */
#define ENBIT (0x02)
#define DELAY40US (200L)
#define DELAY4_1MS (22000L)
#define DELAY100US (5000L)
#define LCDWIDTH (16)

static void lcdDelay(unsigned long constant) {
  volatile unsigned long counter;

  for(counter = constant; counter > 0; counter--);
  
 } /* end lcdDelay() */


/* 
 * Write LCD module in 8-bit mode
 * Inputs:
 *  data: to be written, lower 4 bits are ignored
 *  rs: register select, only bit 0 is significant
 * Handles the shifting into place and the EN pulsing
 * This is only used at the start of the init sequence
 *
 */
static void writeLcd8(unsigned char data) {
  unsigned char temp;

  /* shift upper nibble to data bits in port K */
  temp = (data >> 2); // rs is always 0
  /* Now do the EN pulsing */
  PORTK = temp;         /* write with EN=0 */
  PORTK = temp | ENBIT; /* write with EN=1 */
  PORTK = temp;         /* write with EN=0 */
  /* give it some time for instruction to complete processing */
  lcdDelay(DELAY40US);
} /* end writeLcd8() */


/* 
 * Write LCD module in 4-bit mode
 * Inputs:
 *  data: to be written, 8 bits are significant
 *  rs: register select, only bit 0 is significant
 * Does two consecutive writes, high nibble, then low
 * Handles the shifting into place and the EN pulsing
 * This is can be used at any time (init and display)
 * 
 */
static void writeLcd4(unsigned char data, unsigned char rs) {
  unsigned char hi, lo;

  /* split byte into 2 nibbles and shift to line up
   * with data bits in port K
   */
  hi = ((data & 0xf0) >> 2) | (rs & 0x01) ;
  lo = ((data & 0x0f) << 2) | (rs & 0x01) ;

  /* do write pulses for upper, then lower nibbles */
  PORTK = hi;           /* write with EN=0 */
  PORTK = hi | ENBIT;   /* write with EN=1 */
  PORTK = hi;           /* write with EN=0 */
  PORTK = lo;           /* write with EN=0 */
  PORTK = lo | ENBIT;   /* write with EN=1 */
  PORTK = lo;           /* write with EN=0 */
  /* give it some time for instruction to complete processing */
  lcdDelay(DELAY40US);
} /* end writeLcd4() */


/* 
 * Initialize LCD module. This must be called before
 * any of the other routines is used.
 *
 */
void initLcd(void) {
  /* set port K into output mode */
  DDRK = 0xff;

  writeLcd8(0x30);        /* tell it once */
  lcdDelay(DELAY4_1MS);
  writeLcd8(0x30);        /* tell it twice */
  lcdDelay(DELAY100US);
  writeLcd8(0x30);        /* tell it thrice */

  /* last write in 8-bit mode sets bus to 4 bit mode */
  writeLcd8(0x20);

  /* Now we are in 4 bit mode, write upper/lower nibble */
  writeLcd4(0x28, 0); /* last function set: 4-bit mode, 2 lines, 5x7 matrix */
  writeLcd4(0x0c, 0); /* display on, cursor off, blink off									*/
  writeLcd4(0x01, 0); /* display clear																			*/
  writeLcd4(0x06, 0); /* cursor auto-increment, disable display shift				*/
} /* end initLcd() */


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
void writeLine(char *string, int line) {
	int currentChar;
	char endOfLine;
	unsigned char instruction;

	/* Set address in LCD module */
	if( 1 == line)
		instruction = 0xc0;       /* write bottom line  */
	else
		instruction = 0x80;       /* write top line     */
	writeLcd4( instruction, 0); /* rs=0 means command */

	/* blast out 16 bytes */
  endOfLine = 0;
	for(currentChar = 0; currentChar < LCDWIDTH; ++currentChar) {
	  if (string[currentChar] == 0) {
	   endOfLine = 1;
	  }
	  if (endOfLine == 0) {
	    writeLcd4(string[currentChar], 1); /* rs=1 means data */
	  } 
	  else {
	    writeLcd4(' ', 1); 
	  }
	}
} /* end writeLine() */


