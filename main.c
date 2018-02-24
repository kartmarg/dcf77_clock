/*
	Author: Martin Karg
	Date: 13/06/2016 5:00 PM
	Purpose: Clock in C using DCF77 Signal
*/

#include <mc9s12dp256.h>     /* processor specific definitions */
#include <stdio.h>
#include "lcddrv.h"
#include "led.h"
#include "temp.h"
#include "DCF77.h"
#include "ticker.h"

// Declaration of Constants

// Flags
unsigned char Signal = 0; // To store whether or not a signal has been found 0...1

void main(void) {

	EnableInterrupts;

	// Set PORTH as input port
	DDRH = 0x00;

	// Configures LEDs
	initLED();
	
	// Configures ADC 7
	initADC();

	// Configures LCD
	initLcd();
	
	// Configures and initializes ticker
	initTicker();
	
	for(;;){                            	
		
		if(Signal == 1){
			ledOff(2);
			Signal = normalSignal();
		} else if (Signal == 0) {
			ledOn(2);
			Signal = noSignal();
		}
		
	}
}
