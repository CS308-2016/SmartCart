/*

* Author: Texas Instruments 

* Editted by: Saurav Shandilya, Vishwanathan Iyer 
	      ERTS Lab, CSE Department, IIT Bombay

* Description: This code will familiarize you with using GPIO on TMS4C123GXL microcontroller. 

* Filename: lab-1.c 

* Functions: setup(), ledPinConfig(), switchPinConfig(), main()  

* Global Variables: none

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

#define NO_LED   0
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

#define SWITCH_1 GPIO_PIN_4
#define SWITCH_2 GPIO_PIN_0

/*
 ------ Global Variable Declaration
*/
	

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency and enable GPIO Peripherals  

* Example Call: setup();

*/
void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

/*

* Function Name: ledPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 1, Pin 2, Pin 3 as output.

* Example Call: ledPinConfig();

*/

void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);
}

/*

* Function Name: switchPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.

* Example Call: switchPinConfig();

*/
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=SWITCH_2|SWITCH_1;
	
	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,SWITCH_1|SWITCH_2,GPIO_DIR_MODE_IN);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, SWITCH_1|SWITCH_2);
	GPIOPadConfigSet(GPIO_PORTF_BASE,SWITCH_1|SWITCH_2,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

// Switch 2 status variable
uint32_t sw2Status=0;

int main(void)
{
	// Setup the various pins
	setup();
	ledPinConfig();
	switchPinConfig();

	// Clicked status variable
	uint8_t clicked = 0;
	uint8_t switch2_clicked = 0;

	// Current LED Color State
	uint8_t ledColor=NO_LED;

	// Press variable to get the press state
	uint32_t switchPress=0;

	while(1){
		// Switch 1 click handler
		switchPress = GPIOPinRead(GPIO_PORTF_BASE,SWITCH_1);
	    if((switchPress & SWITCH_1) == 0) {
	    		// Switch 1 is pressed
	    		if (clicked == 0) {
	    			// Switch 1 wasn't pressed in the previous cycle => Click just happened
	    			// Change the color of the LED

	    			if (ledColor == NO_LED) {
	    				// NO_LED => RED_LED
	    				ledColor = RED_LED;
	    			} else if (ledColor == RED_LED) {
	    				// RED_LED => GREEN_LED
	    				ledColor = GREEN_LED;
	    			} else if (ledColor == GREEN_LED) {
	    				// GREEN_LED => BLUE_LED
	    				ledColor = BLUE_LED;
	    			} else if (ledColor == BLUE_LED) {
	    				// BLUE_LED => RED_LED
	    				ledColor = RED_LED;
	    			}

	    			// Set clicked started variable to 1
	    			clicked = 1;
	    		}

	    		// Turn on the LED Light
	    		GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, ledColor);
	    } else {
	    		// Set the clicked variable to unclicked
	    		clicked = 0;

	    		// Turn off the LED
	    		GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, NO_LED);
	    }

	    // Switch 2 Click Handler
	    /*switchPress = GPIOPinRead(GPIO_PORTF_BASE,SWITCH_2);
	    if((switchPress & SWITCH_2) == 0) {
	    		// Switch 2 was clicked, increment the status variable
	    		sw2Status++;
	    }*/


	    switchPress = GPIOPinRead(GPIO_PORTF_BASE,SWITCH_2);
	    if((switchPress & SWITCH_2) == 0) {
	    		// Switch 2 was clicked, increment the status variable
	    		if (switch2_clicked == 0) {
	    			sw2Status++;
		    		switch2_clicked = 1;
	    		}
	    } else {
	    		switch2_clicked = 0;
	    }


	    // Delay between cycles
	    SysCtlDelay(1000000);
	}

}
