#include<stdint.h>
#include<stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include"inc/hw_memmap.h"
#include"inc/hw_types.h"
#include"driverlib/debug.h"
#include"driverlib/sysctl.h"
#include"driverlib/adc.h"
#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include"driverlib/uart.h"
#include"utils/uartstdio.h"
#include"driverlib/pin_map.h"

#define qMode 2
#define MONITORING 1

uint8_t qMode2 = MONITORING;
uint32_t setTemperature = 25;

// The LEDs
#define LED_R GPIO_PIN_1
#define LED_B GPIO_PIN_2
#define LED_G GPIO_PIN_3

uint32_t ui32Period;

uint32_t ui32ADC0Value[4];

volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

int val = 0;

void UARTIntHandler(void) {
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		char a = UARTCharGetNonBlocking(UART0_BASE);
		UARTCharPutNonBlocking(UART0_BASE, a); //echo character
		if (qMode == 2) {
			if (qMode2 == MONITORING) {
				if (a == 's') {
					qMode2 = 2;
					UARTprintf("Enter the temperature ");
				}
			} else {
				int x = a - '0';
				if (x >= 0 && x <= 9) {
					val = val*10 + x;
				} else {
					qMode2 = MONITORING;
					setTemperature = val;
					val = 0;
				}
			}
		}
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
		SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
	}
}

void getTemp(void) {
	ADCIntClear(ADC0_BASE, 1);
	ADCProcessorTrigger(ADC0_BASE, 1);

	while(!ADCIntStatus(ADC0_BASE, 1, false)) {
	}

	ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);

	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
	ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
	ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
}

void Timer0IntHandler(void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	getTemp();
	if (qMode == 1) {
		UARTprintf("Current temperature %d %cC\n", ui32TempValueC, 176);
	} else {
		if (qMode2 == MONITORING) {
			UARTprintf("Current temp %d %cC, Set Temp %d %cC\n", ui32TempValueC, 176, setTemperature, 176);
			if (setTemperature < ui32TempValueC) {
				GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, LED_R);
			} else {
				GPIOPinWrite(GPIO_PORTF_BASE, LED_R|LED_B|LED_G, LED_G);
			}
		}
	}
}

int main(void) {
	
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	/*Timer Enable*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_R|LED_B|LED_G);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet() / 1) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
	/*Timer Enable*/

	/*ADC Enable*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	ADCHardwareOversampleConfigure(ADC0_BASE, 64);

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);

	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);

	ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

	ADCSequenceEnable(ADC0_BASE, 1);
	/*ADC Enable*/

	/*UART Enable*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); //enable pin for LED PF2
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	IntMasterEnable(); //enable processor interrupts
	IntEnable(INT_UART0); //enable the UART interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
	/*UART Enable*/

	UARTStdioConfig(0, 115200, SysCtlClockGet());

	while (1) {}

}
