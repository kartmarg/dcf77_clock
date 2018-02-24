/*
	Author: Martin Karg
	Date: 13/06/2016 6:45 PM
	Purpose: Clock with DCF77 driver
*/

void tick10ms();
unsigned char getDCF77();
void processTime();
int noSignal();
int normalSignal();
void processDCF77();
int validateDCF77();
void getSW2();
void refreshDisplay(int temp);
