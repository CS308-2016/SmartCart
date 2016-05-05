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
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"

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

#define MONITOR_MODE 1
#define SET_MODE 2

#define STATE_IDLE 1
#define STATE_PRESSED 2
#define STATE_HOLD 3
#define STATE_RELEASED 4

uint32_t ui32ADC0Value[4];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

volatile uint32_t setTemperature = 25;
uint8_t mode = MONITOR_MODE;
uint8_t value = 0;
uint8_t question = 1;
uint8_t stateSwitch1 = STATE_IDLE;
uint8_t stateSwitch2 = STATE_IDLE;


void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}


void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);
}

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

void p(char a) {
	UARTCharPutNonBlocking(UART0_BASE, a);
}

// Set the LED
void setLED(uint8_t color) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, color);
}
void setNoLED(void) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, NO_LED);
}
void setRedLED(void) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
}
void setGreenLED(void) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, GREEN_LED);
}
void setBlueLED(void) {
	GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
}

// Enables the Timer
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

// UART Interrupt handler
void UARTIntHandler(void) {
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		char input = UARTCharGetNonBlocking(UART0_BASE);
		if (question == 2) {
			if (mode == MONITOR_MODE) {
				if (input == 's') {
					mode = SET_MODE;
					UARTprintf("Enter The Temperature ");
				}
			} else {
				int a = input - '0';
				if (a >= 0 && a<= 9) {
					value *= 10;
					value += a;
				} else {
					setTemperature = value;
					value = 0;
					mode = MONITOR_MODE;
				}
			}
		}
		UARTCharPutNonBlocking(UART0_BASE, input); //echo character
		setBlueLED();
		SysCtlDelay(SysCtlClockGet() / (50 * 3)); //delay ~1 msec
		setNoLED();
	}
}

void handleSwitchPress(uint8_t swich_pin, uint8_t* state) {

	uint8_t current_state = *state;
	if(GPIOPinRead(GPIO_PORTF_BASE,swich_pin)==0x00)
	{
		if (current_state == STATE_IDLE) {
			*state = STATE_PRESSED;
		} else if (current_state == STATE_PRESSED) {
			*state = STATE_HOLD;
		}
	} else {
		if (current_state == STATE_PRESSED || current_state == STATE_HOLD) {
			*state = STATE_RELEASED;
		} else if (current_state == STATE_RELEASED) {
			*state = STATE_IDLE;
		}
	}
}

// Enables the UART
void enableUART(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	IntMasterEnable();
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}

// Enables the ADC
void enableADC(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

	ADCSequenceEnable(ADC0_BASE, 1);
}

// Gets the Temperature value and sets the global variables
void getADCReading(void) {
	// Clear the ADC Interrupt Buffer
	ADCIntClear(ADC0_BASE, 1);

	// Request Value
	ADCProcessorTrigger(ADC0_BASE, 1);

	while(!ADCIntStatus(ADC0_BASE, 1, false)) {
		// Busy Wait
	}
	ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
	ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
	ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
}

// Code for problem 1
void handleProblem1(void)
{
	getADCReading();
	UARTprintf("Current Temperature %d %cC\n", ui32TempValueC, 176);
}

// Code for problem 2
void handleProblem2(void)
{
	getADCReading();
	if (mode == MONITOR_MODE) {
		UARTprintf("Current Temp %d %cC, Set Temp %d %cC\n", ui32TempValueC, 176, setTemperature, 176);
		if (setTemperature > ui32TempValueC) {
			setGreenLED();
		} else {
			setRedLED();
		}
	} else {
		// Do nothing, handled in the Interrupt code
	}
}


void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	handleSwitchPress(SWITCH_1, &stateSwitch1);
	handleSwitchPress(SWITCH_2, &stateSwitch2);
	if (stateSwitch1 == STATE_HOLD) {
		question = 1;
	} else if (stateSwitch2 == STATE_HOLD) {
		question = 2;
	}

	if (question == 1) {
		handleProblem1();
	} else {
		handleProblem2();
	}
}

int main(void)
{
	// Setup the various pins
	setup();
	ledPinConfig();
	switchPinConfig();
	enableTimer();
	enableADC();
	enableUART();

	UARTStdioConfig(0, 115200, SysCtlClockGet());

	while(1){
		// Delay between cycles
	}

}
