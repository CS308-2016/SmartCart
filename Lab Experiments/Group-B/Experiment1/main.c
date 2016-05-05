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

// The LEDs
#define LED_R GPIO_PIN_1
#define LED_B GPIO_PIN_2
#define LED_G GPIO_PIN_3

// The switches
#define SW_1 GPIO_PIN_4
#define SW_2 GPIO_PIN_0

/*
Global Variable Declaration
*/
uint32_t sw2Status = 0;

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
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_R|LED_B|LED_G);
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
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE, SW_1|SW_2, GPIO_DIR_MODE_IN);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, SW_1|SW_2);
	GPIOPadConfigSet(GPIO_PORTF_BASE, SW_1|SW_2, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD_WPU);
}

int main(void)
{
	setup();
	ledPinConfig();
	switchPinConfig();

	uint32_t switch1Val=0;
	uint32_t switch2Val=0;
	uint8_t ledVal=2;
	uint32_t clicked = 0;
	while(1){
		switch1Val = GPIOPinRead(GPIO_PORTF_BASE,SW_1);
		if( (switch1Val & SW_1)==0) {
			if (clicked == 0) {
				if (ledVal == LED_R) {
					ledVal = LED_G;
				} else if (ledVal == LED_G) {
					ledVal = LED_B;
				} else if (ledVal == LED_B) {
					ledVal = LED_R;
				}
			}
			clicked = 1;
			GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, ledVal);
		} else {
			clicked = 0;
			GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, 0);
		}

		switch2Val = GPIOPinRead(GPIO_PORTF_BASE, SW_2);
		if ( (switch2Val && SW_2)==0 ) {
			sw2Status++;
		}
		SysCtlDelay(1000000);
	}
}
