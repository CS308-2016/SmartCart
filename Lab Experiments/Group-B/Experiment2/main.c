#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

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

uint32_t sw2Status = 0;

uint32_t s1clicked = 0;
uint32_t s2clicked = 0;

uint32_t s1prev = 0;
uint32_t s2prev = 0;

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
	uint32_t ui32Period;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	switchPinConfig();

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet() / 1) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);

	while(1) {}
}

uint8_t ledVal=0;

/*
 *
 */

void Timer0IntHandler(void) {
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	uint32_t switch1Val=0;
	uint32_t switch2Val=0;
	switch1Val = GPIOPinRead(GPIO_PORTF_BASE,SW_1);
	if( (switch1Val & SW_1)==0) {
		if (s1clicked == 0) {
			s1clicked = 1;
		} else if (s1clicked == 1) {
			s1clicked = 2;
			if (ledVal == LED_R) {
				ledVal = LED_G;
			} else if (ledVal == LED_G) {
				ledVal = LED_B;
			} else {
				ledVal = LED_R;
			}
			GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, ledVal);
		}
	} else {
		s1clicked = 0;
		GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, 0);
	}

	switch2Val = GPIOPinRead(GPIO_PORTF_BASE, SW_2);
	if ( (switch2Val && SW_2)==0 ) {
		if (s2clicked == 0) {
			s2clicked = 1;
		} else if (s2clicked == 1) {
			s2clicked = 2;
			sw2Status++;
		}
	} else {
		s2clicked = 0;
	}
}
