/*
	Author: Martin Karg
	Date: 13/06/2016 6:45 PM
	Purpose: Led Driver
*/

void initLED ();
void setLeds(unsigned char set_leds);
unsigned char getLeds();
void ledOn(unsigned char led_number);
void ledOff(unsigned char led_number);
void ledToggle(unsigned char led_number);
