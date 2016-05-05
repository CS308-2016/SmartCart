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
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

#define NO_LED   0
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

#define SWITCH_1 GPIO_PIN_4
#define SWITCH_2 GPIO_PIN_0

#define KEY_STATE_IDLE 1
#define KEY_STATE_PRESS 2
#define KEY_STATE_RELEASE 3

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



void enableTimer(void) {
	uint32_t ui32Period;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet() / 1) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
}

uint8_t switch1State = KEY_STATE_IDLE;
uint8_t switch2State = KEY_STATE_IDLE;

uint8_t detectKeyPress(uint32_t switchPin, uint8_t* statePointer) {
	uint8_t state = *statePointer;
	uint32_t switchPress = GPIOPinRead(GPIO_PORTF_BASE,switchPin);
	if((switchPress & switchPin) == 0) {
		if (state == KEY_STATE_IDLE) {
			*statePointer = KEY_STATE_PRESS;
		} else if (state == KEY_STATE_PRESS) {
			*statePointer = KEY_STATE_RELEASE;
			return 1;
		}
	} else {
		if (state == KEY_STATE_RELEASE) {
			*statePointer = KEY_STATE_IDLE;
			return 2;
		}
		*statePointer = KEY_STATE_IDLE;
	}

	return 0;
}

uint8_t ledColor=NO_LED;
void handleSwitch1Click() {
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
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, ledColor);
}

void handleSwitch1Raise(void) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, NO_LED);
}

int sw2Status = 0;
void handleSwitch2Click() {
	sw2Status++;
}

void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	uint8_t switch1Response = detectKeyPress(SWITCH_1, &switch1State);
	if (switch1Response == 1) {
		handleSwitch1Click();
	} else if (switch1Response == 2) {
		handleSwitch1Raise();
	}
	if (detectKeyPress(SWITCH_2, &switch2State) == 1) {
		handleSwitch2Click();
	}
}

int main(void)
{
	// Setup the various pins
	setup();
	ledPinConfig();
	switchPinConfig();
	enableTimer();

	while(1){
	    // Delay between cycles
	}

}
