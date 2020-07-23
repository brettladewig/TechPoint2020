//***************************************************************************
// TechPoint Challenge 2020
// C code to accomplish the following:
//
// * Required Components: MSP430fr6989, LCD Display, Push Buttons
//
//	 Functionality: this program is designed to be able to control the brightness
//					an LED as well as create a custom timer with an alarm
// Brett Ladewig
// 7/19/2020   ver 4.20
//
// I have neither given or recieved, nor have i tolorated others use of unauthorized aid
//***************************************************************************
//***************************************************************************
// Includes
//***************************************************************************
#include <msp430.h>
#include <stdlib.h>
#include <driverlib.h> 		   // Required for the LCD
#include <time.h>			   // Required for random numbers
#include <string.h>			   // used in scrollwords()
#include "myGpio.h" 		   // Required for the LCD
#include "myClocks.h" 		   // Required for the LCD
#include "myLcd.h" 			   // Required for the LCD

//***************************************************************************
// global variables
//***************************************************************************
int timer = 10;
//***************************************************************************
// Function Prototypes
//***************************************************************************
void ScrollWords();
void ADC_SETUP();
void timercountdown(int timer);
void delay_ms(unsigned int x);

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;					// Stop watchdog timer
    PM5CTL0 = 0xFFFE;          					// Unlock I/O pins

    initGPIO();									// Inputs and Outputs for LCD
    initClocks(); 								// Initialize clocks for LCD

    myLCD_init(); 								// Initialize the LCD

    P1DIR = P1DIR | BIT0; 						// Set pin for red LED as an output
    P1OUT = P1OUT & ~BIT0; 						// Make sure red LED is off to start
    P9DIR = P9DIR | BIT7; 						// Set pin for green LED as an output
    P9OUT = P9OUT | BIT7; 						// Make sure green LED is off to start

    P1DIR = P1DIR & ~BIT1;
    P1DIR = P1DIR & ~0x06;						// set up both buttons as
    P1REN = P1REN | 0x06;						// inputs with pull up
    P1OUT = P1OUT | 0x06;						// resistors

	ScrollWords("WELCOME      PRESS THE LEFT BUTTON TO SET TIMER      PRESS THE RIGHT BUTTON TO START THE TIMER      USE THE KNOB TO TURN ON THE LIGHT");
	myLCD_displayNumber(timer);

	P1IE = BIT1 | BIT2;							// setup interrupts for both buttons
	P1IES = BIT1 | BIT2;
	P1IFG = 0x00;

    _BIS_SR(GIE); 				//enable interrupts

    while(1);

    }

void timercountdown(int timer){

	//every second, count timer down one; display new on timer until 0; buzz after with red led/piezo
	int i = timer;

	for( i; i >= 1; i--){
		delay_ms(1500);
		timer--;
		myLCD_displayNumber(timer);
	}

	ScrollWords("TIME IS UP");
	P1OUT = P1OUT | BIT0;
	delay_ms(500);
	P1OUT = P1OUT & ~BIT0;

}


void ScrollWords(char words[300]) {
    unsigned int length; // Contains length of message to be displayed
    unsigned int slot; // Slot to be displayed on LCD (1, 2, 3, 4,
    // 5, or 6)
    unsigned int amount_shifted; // Number of times message shifted so far
    unsigned int offset; // Used with amount_shifted to get correct
    // character to display
    unsigned long delay; // Used to implement delay between scrolling
    // iterations
    unsigned char next_char; // Next character from message to be
    // displayed
    length = strlen(words); // Get length of the message stored in words
    amount_shifted = 0; // We have not shifted the message yet
    offset = 0; // There is no offset yet

    int cancel_prompt = 0;

    while ((amount_shifted < length + 7)&&(!cancel_prompt)){ // Loop as long as you haven't shifted all
    														// of the characters off the LCD screen
        offset = amount_shifted; // Starting point in message for next LCD update
        for (slot = 1; slot <= 6; slot++) // Loop 6 times to display 6 characters at a time
                {
            next_char = words[offset - 6]; // Get the current character for LCD slot
            if (next_char && (offset >= 6) && (offset <= length + 6)) // If character is not null AND
                    { // LCD is not filled (offset>=6) AND
                      // You have not reached end of message
                      // (offset<=length+6)
                myLCD_showChar(next_char, slot); // Show the next character on the LCD
                // screen in correct slot
            }  //end if
            else // Else, slot on LCD should be blank
            {
                myLCD_showChar(' ', slot); // So, add a blank space to slot
            } //end else
            offset++; // Update as you move across the message
        } //end for
        for (delay = 0; delay < 63456; delay = delay + 1)
            ; // Delay between shifts
        amount_shifted = amount_shifted + 1; // Update times words shifted across LCD
    } //end while
}

void delay_ms(unsigned int x) {
	TA2CCR0 = 1000; // Set timer TA2 for 1 ms using SMCLK
	TA2CTL = 0x0214; // Start timer TA2 from zero in UP mode using SMCLK
	while (1) // Loop
	{
		if (TA2CTL & BIT0) // Is TA2 flag set?
		{ // if yes do
			TA2CTL = TA2CTL & ~BIT0; // Reset TA2 flag (TAIFG)
			x--; // Decrement interval counter (# ms)
			if (x == 0)
				break; // if counter = 0 break out of loop
		} //end if(flag) // we have reached x milliseconds
	} //end while(1)
	TA2CTL = 0x0204; // Stop timer TA2
} //end delay_ms(x)

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {


	// Simple delay debounce
	// unsigned long delay; // Wait for bouncing to end
	// for(delay=0 ; delay<12345 ; delay=delay+1);

	// Better "best of" debounce
	unsigned char k;
	unsigned char debounce = 0;
	unsigned int i;

	for (k = 0; k < 15; k++){ 					// check buttons 15 times
		for (i = 0; i < 100; i++);				// delay to check buttons (~1ms)
		if (!(P1IN & 0x02) | !(P1IN & 0x04))
			debounce++; 						// if pressed count
	} 											//end for(k)

	if (debounce > 5){ 							// pressed 5 of 15 times so a button is pressed (P1.1 or P 1.2)

												// Check which button interrupted (P1.1 or P1.2) and toggle appropriate LED
		switch (P1IV){ 							// What is stored in P1IV?
			case 4: { 							// Come here if P1.1 interrupt
			timer = timer + 10; 				//Add to the timer
			myLCD_displayNumber(timer);				//Display the timer number
			if (timer >= 120){
				timer = 0;
			}
			break;
			}
			case 6: { 							// Come here if P1.2 interrupt
			if(timer > 0){
			timercountdown(timer);
			}
				break; 							// Then leave switch
			}

		} 										// end switch statement

	} 											//end if

	if (P1IV); 										// must read port interrupt vector to reset the highest pending interrupt

} 												// end ISR

