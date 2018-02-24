/*
	Author: Martin Karg
	Date: 13/06/2016 6:45 PM
	Purpose: ADC Driver
*/

#include <mc9s12dp256.h>
#include <stdio.h>
#include <temp.h>

// Temp global variable
int Temp = 0; // Stores temperature data -40...255

//******************Init ADC********************//
// Initializes ADC port
void initADC(){
	ATD0CTL2 = 0x80;  //Turn on ADC
	ATD0CTL3 = 0x08;  //one conversion, no FIFO
	ATD0CTL4 = 0xEB;  //8-bit resolution
}

//***************Read Temperature*****************//
// Reads ADC Port 7 and transforms the data read into an int value which ranges from
// -40 to 40 (which is the min/max temperature that can be read). Has 8-bit precision
int readTemperature(){
	ATD0CTL5 = 0x87;
	while(!(ATD0STAT0 & 0x80));
	// Variable Temp is equal to the ADC input
	Temp = ATD0DR0L;
	// Variable Temp is converted to a temperature in the range of
	// -40...40 degrees Celsius
	Temp = ((Temp - 128) * 40) / 127;
	return Temp;
}

