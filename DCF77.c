/*
	Author: Martin Karg
	Date: 13/06/2016 6:45 PM
	Purpose: DCF77 Clock driver
*/

#include <mc9s12dp256.h>
#include <stdio.h>
#include "lcddrv.h"
#include "led.h"
#include "temp.h"
#include "dcf77.h"

// Declaration of Constants

// LED Constants
unsigned char Portb0 = 0;
unsigned char Portb1 = 1;
unsigned char Portb2 = 2;
unsigned char Portb3 = 3;
unsigned char Portb4 = 4;
unsigned char Portb5 = 5;
unsigned char Portb6 = 6;
unsigned char Portb7 = 7;

// Day of Week String Constants
const char *DAY_WEEK_STR[7] = {
  "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

// Month String Constants
const char *MONTH_STR[12] = {
  "Jan", "Feb","Mar","Apr","May""Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

// Number of Days in Month
const char DAYS_MONTH[12] = {
  31,28,31,30,31,30,31,31,30,31,30,31
};

// Time global variables
unsigned char Hour = 23; // Variable where hours are stored. 0...23
unsigned char Min = 59;	// Variable where minutes are stored. 0...59
unsigned char Sec = 0;	// Variable where seconds are stored. 0...59

unsigned char Day = 31; // Variable where days are stored. 1...31
unsigned char Month = 12; // Variable where months are stored. 1...12
unsigned char Year = 0; // Variable where years are stored. 0...99

unsigned char Day_Week = 1; // Variable where current day of the week is stored 1...7

// Time global variables for use in DCF77 validation/storage
unsigned char HourDC = 23; // Variable where hours are stored. 0...23
unsigned char MinDC = 59;	// Variable where minutes are stored. 0...59
unsigned char SecDC = 0;	// Variable where seconds are stored. 0...59

unsigned char DayDC = 31; // Variable where days are stored. 1...31
unsigned char MonthDC = 12; // Variable where months are stored. 1...12
unsigned char YearDC = 0; // Variable where years are stored. 0...99

unsigned char Day_WeekDC = 1; // Variable where current day of the week is stored 1...7

// Array where all DCF77 data is stored
int Bit_Read[60]; // Each location: 0...1

// Flags
unsigned char MS_10 = 0; // To check whether or not 10 ms have passed 0...1
unsigned char Hour12 = 0; // To store 12 hour mode state 0...1
unsigned char PM = 0; // To store am/pm status 0...1 = AM...PM
unsigned char Sec_Start = 0; // To check if the SW2 button was already pressed before 0...1

// Counter global variables
int Current_Bit = 0; // For current bit read from DCF77 0...59
int S_1 = 0; // For counting amount of times 10 ms have passed 0...100
int Low_Count = 0; // To count the amount of times that after 10 ms the DCF77 signal has been LOW 0...100
int High_Count = 0; // To count the amount of times that after 10 ms the DCF77 signal has been HIGH 0...100

// Strings for the LCD display
char Time[17]; // To store formatted time string "hh:mm:ss AM/PM/NA tt C"
char Date[17]; // To store formatted date string "day_week day.month.year"

//***************Switch On MS_10 flag***/***********//
// This function is called periodically every 10ms by the ticker interrupt.
// Callback function, never called by user directly.
void tick10ms(){   
	MS_10 = 1;
}

//*****************Get DCF77 Signal****************//
// Gets the DCF77 signal at H0 and returns 1 if the input
// is HIGH and 0 if the input is LOW
unsigned char getDCF77(){
	// Checks port h to see if input PORTH0 is 1 or 0
	//HIGH
	if (PTIH & 0x01 == 0x01) {
		return 1;
	} 
	//LOW
	else {
		return 0;
	}
}

//**************Process Time (Seconds)*************//
// Process the time (sec, min, and hour), and the date
// (day of week, day, month and year)
void processTime(){
	if (Sec > 59){
		Sec = 0;
		Min = Min + 1;
		if (Min > 59)
		{
			Min = 0;
			Hour = Hour + 1;
			if (Hour > 23)
			{
				Hour = 0;
				Day = Day + 1;
				if(Day > DAYS_MONTH[Month-1]){
					Day = 1;
					Month = Month + 1;
					if(Month > 12){
						Month = 1;
						Year = Year + 1;
					}
				}
				Day_Week = Day_Week + 1;
				if(Day_Week > 7){
					Day_Week = 1;
				}
			}
		}
	}
}

//**************No signal w/clock count*************//
// Counts the amount of times 10 ms pass and checks the current state of 
// DCF77 input to find the +1000 ms period where the input stays at HIGH
// Once that period happens, the system knows that next time it reads
// a LOW state from the DCF77 input will be the beginning of the data stream
// Return 0 if no signal was found and 1 if the signal was found
int noSignal(){
	if(MS_10 == 1){
		
		// Resets 10 MS flag
		MS_10 = 0;
		
		// Add 1 to S_1 counter, which is used to determine the amount of 
		// times 10 ms have passed
		S_1++;
		
		// Checks current DCF77 state (at 10 ms mark), and then adds 1
		// to either LOW state or HIGH state counters
		if(getDCF77() == 0){
			// Input is low -> B1 = ON
			ledOn(Portb1);
			
			// Resets high state counter and adds 1 to low state counter
			Low_Count++;
			High_Count = 0;
		} else if (getDCF77() == 1) {
			// Input is high -> B1 = OFF
			ledOff(Portb1);
			
			High_Count++;
			Low_Count = 0;
		}
		
		// Checks if one second has passed (using the S_1 counter), and knowing
		// that 100 * 10 ms = 1000 ms = 1 s
		if ( !(S_1 <= 100)){
			
			// Gets the current status of button SW2 and checks wheter to toggleLED
			// 12 hour mode
			getSW2();
			
			// Toggles LED B0 as one second has passed
			ledToggle(Portb0);
			
			// Adds 1 to the current second counter
			Sec = Sec + 1;
			
			// Resets S_1 counter
			S_1 = 0;
			
			// If the amount of 10 ms that have passed where the state of the signal
			// was HIGH exceeds 100 (which is 1000 ms), then we know that the current bit
			// is either bit 59 so the next bit must be bit 0, so we have found
			// the correct signal and normal mode with signal may resume. This happens 
			// because in any two bits that are not 58/59 the maximum time at state
			// HIGH (w/o changes to LOW) is 900 ms
			if (High_Count > 100){
				Current_Bit = 0;
				
				// Reset HIGH and LOW counters
				High_Count = 0;
				Low_Count = 0;
				
				// Signal start found
				return 1;
				
			} else {
				
				// Refreshes the display
		    	// Processes the time to be printed into the display
			    processTime();
			    refreshDisplay(readTemperature());
				
				return 0;
				
			}
		}
	  
		return 0;
	}
	return 0;
	
}

//***************Signal w/ clock count**************//
// Counts the amount of times 10 ms pass and saves the data from the DCF77
// input into the Bit_Read array, then when it reaches the last bit it processes
// and validates the read data stream
// Returns 1 if there's a signal/data was corrupt, 0 if there's no signal
int normalSignal(){
	if(MS_10 == 1){
		
		// Resets 10 MS flag
		MS_10 = 0;
		
		// Add 1 to S_1 counter, which is used to determine the amount of 
		// times 10 ms have passed
		S_1++;
		
		// Checks current DCF77 state (at 10 ms mark), and then adds 1
		// to either LOW state or HIGH state counters
		if(getDCF77() == 0){
			// Input is low -> B1 = ON
			ledOn(Portb1);
			
			// Resets high state counter and adds 1 to low state counter
			Low_Count++;
		} else if (getDCF77() == 1) {
			// Input is high -> B1 = OFF
			ledOff(Portb1);
			
			// Adds 1 to high state counter
			High_Count++;
		}
		
		// Checks if one second has passed (using the S_1 counter), and knowing
		// that 100 * 10 ms = 1000 ms = 1 s
		if ( !(S_1 <= 100)){
		
			// Gets the current status of button SW2 and checks wheter to toggleLED
			// 12 hour mode
			getSW2();
		
			// Toggles LED B0 as one second has passed
			ledToggle(Portb0);
			
			// Adds 1 to the current second counter
			Sec = Sec + 1;
			
			// Resets S_1 counter
			S_1 = 0;
			
			// If the amount of 10 ms that have passed where the state of the signal
			// was LOW exceeds or is equal to 150 ms (which is 15 times 10 ms), then 
			// the read bit is a 0, and if the LOW period is lesser than 150 ms then
			// the read bit is a 1
			if (Low_Count > 7 && Low_Count <= 13){
				Bit_Read[Current_Bit] = 0;
			} 
			else if (Low_Count > 13){
				Bit_Read[Current_Bit] = 1;
			}
			
			// Reset HIGH and LOW counters
			High_Count = 0;
			Low_Count = 0;
			
			// Adds 1 to Current_Bit so the next bit read is put into the next location
			// of the Bit_Read array
			Current_Bit = Current_Bit + 1;
			
			if (Current_Bit > 58){
				Current_Bit = 0;
				
				// Readies the system to process a complete DCF77 signal
				processDCF77();
				
				// Processes time
				processTime();
				
				if(validateDCF77() == 1){
					// Signal decoded correctly -> B3 = ON
					ledOn(Portb3);
					// No errors in data for DCF77 -> B2 = OFF
					ledOff(Portb2);
					Sec = 0;
					
					// Refreshes the display
					// Processes the time to be printed into the display
					processTime();
					refreshDisplay(readTemperature());
					
					return 1;
				} else {
					// Errors in data for DCF77 -> B2 = ON
					ledOn(Portb2);
					// Signal decoded incorrectly -> B3 = OFF
					ledOff(Portb3);
					
					// Refreshes the display
		      		// Processes the time to be printed into the display
		      		processTime();
		      		refreshDisplay(readTemperature());
					
					return 0;
				}
			}
			
			// Refreshes the display
	  		// Processes the time to be printed into the display
	  		processTime();
	  		refreshDisplay(readTemperature());
		
			return 1;
			
			// To-do: should probably reset the 10 ms ticker at this time so the clock is more exact
			// and should ledToggle
		}
		
		return 1;
	}
	
	return 1;
}

//*******************Process DCF77******************//
// Transforms the information received from the DCF77 (which is saved into the...
// Bit_Read array) into the variables: Min, Hour, Day, Day_Week, Month, Year
void processDCF77(){

	MinDC = 	Bit_Read[21] + (2 * Bit_Read[22]) + (4 * Bit_Read[23]) + (8 * Bit_Read[24]) + 
				(Bit_Read[25] * 10) + (20 * Bit_Read[26]) + (40 * Bit_Read[27]);
		  
	HourDC = 	Bit_Read[29] + (2 * Bit_Read[30]) + (4 * Bit_Read[31]) + (8 * Bit_Read[32]) + 
				(10 * Bit_Read[33]) + (20 * Bit_Read[34]);
		  
	DayDC = 	Bit_Read[36] + (2 * Bit_Read[37]) + (4 * Bit_Read[38]) + (8 * Bit_Read[39]) + 
				(10 * Bit_Read[40]) + (20 * Bit_Read[41]);
		  
	Day_WeekDC = Bit_Read[42] + (2 * Bit_Read[43]) + (4 * Bit_Read[44]);
	
	MonthDC =	Bit_Read[45] + (2 * Bit_Read[46]) + (4 * Bit_Read[47]) + (8 * Bit_Read[48]) + 
				(10 * Bit_Read[49]);
			
	YearDC =	Bit_Read[50] + (2 * Bit_Read[51]) + (4 * Bit_Read[52]) + (8 * Bit_Read[53]) + 
				(10 * Bit_Read[54]) + (20 * Bit_Read[55]) + (40 * Bit_Read[56]) + (80 * Bit_Read[57]);

}

//******************Validate DCF77*****************//
// Returns 0 if data is not valid
// Returns 1 if data is valid
int validateDCF77(){
	if(HourDC >= 0 && HourDC <= 23){
		Hour = HourDC;
		if(MinDC >= 0 && MinDC <= 59){
			Min = MinDC;
			if(DayDC >= 1 && DayDC <= 31){
				Day =DayDC;
				if(Day_WeekDC >= 1 && Day_WeekDC <= 7){
					Day_Week = Day_WeekDC;
					if(MonthDC >= 1 && MonthDC <= 12){
						Month = MonthDC;
						if(YearDC >= 0 && YearDC <= 99){
							Year = YearDC;
							Sec = 0;
							return 1;
						} else {
							Sec = 0;
							return 0;
						}
					} else {
						Sec = 0;
						return 0;
					}
				} else {
					Sec = 0;
					return 0;
				}
			} else {
				Sec = 0;
				return 0;
			}
		} else {
			Sec = 0;
			return 0;
		}
	} else {
		Sec = 0;
		return 0;
	}
}

//******************Validates 12 Hour Switch*****************//
//
void getSW2(){
	
	unsigned char SW2;
	
	SW2 = PTIH & 0x08;
	
	// If SW2 is HIGH, the flag for pressed is turned on, because H0  is changing constatntly
	// because of the DCF77 signal, then the processing must be done disregarding that bit
	if (SW2 == 0){
		// If SW2 is high, and the flag for pressed was 0, then the flag for pressed 
		// is turned to 1
		if (Sec_Start == 0){
			Sec_Start = 1;
		} 
		// Else if SW2 is HIGH and the flag for pressed was already 1 (from last second)
		// then the flag is reset and Hour12 is toggled
		else {
			// Toggles Hour12 between 0 and 1
			Hour12 ^= 1 << 0; 
			Sec_Start = 0;
		}
	} 
	// Else if SW2 is LOW
	else if (SW2 != 0){
		// If the flag was turned on last second, then the flag is reset as the button was not
		// kept pressed
		if (Sec_Start == 1){
			Sec_Start = 0;
		}
	}
}

//****************Refresh Display****************//
void refreshDisplay(int temp){
	//Hour to be printed in the display
	int hp;
	
	hp = Hour;
	
	// 12 Hour mode is active
	if (Hour12 == 1){
		// Validation for 00 hours to be 12 AM
		if (Hour == 0) {
			hp = 12;
		} 
		// Validation for hours larger than 12 hours to be displayed 
		// in 12 hour format (i.e 13 hours = 1 PM)
		else if (Hour > 12){
			hp = Hour - 12;
		}
		
		// Validation for AM string
		if(Hour >= 0 && Hour <= 11){
			PM = 0;
			sprintf(Time, "%01i:%01i:%01i AM %01i C", hp, Min, Sec, temp);
		} 
		// Validation for PM string
		else {
			PM = 1;
			sprintf(Time, "%01i:%01i:%01i PM %01i C", hp, Min, Sec, temp);
		}
	} 
	// 12 hours mode is not active
	else {
		//CORRECT: 
		sprintf(Time, "%01i:%01i:%01i %01i C", hp, Min, Sec, temp);
		//DEBUG: 
		//sprintf(Time, "%01i:%01i:%01i %01i %i", hp, Min, Sec, Bit_Read[0], 0);
	}
	
	// Format date string to be printed in LCD
	sprintf(Date, "%s %i.%i.20%02i", DAY_WEEK_STR[Day_Week-1], Day, Month, Year);
	
	// Print in LCD
	writeLine(Time, 0);
	writeLine(Date, 1);
}