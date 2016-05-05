#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"


#define PWM_FREQUENCY 55

uint32_t speed = 20;
uint8_t rg = 1;
uint8_t gb = 0;
uint8_t br = 0;
int auto_flag = 0;
int manual_flag = 0;

int sw1Press = 0;
int sw1Hold = 0;
int sw1Idle = 1;
int sw1LongHold = 0;
int sw1PressCount = 0;

int sw2Press = 0;
int sw2Hold = 0;
int sw2Idle = 1;
int sw2LongHold = 0;

int manual1 = 0;
int manual2 = 0;
int manual3 = 0;

volatile uint8_t ui8Adjust1;
volatile uint8_t ui8Adjust2;
volatile uint8_t ui8Adjust3;

int main(void)
{
	volatile uint32_t ui32Load;
	volatile uint32_t ui32PWMClock;
	ui8Adjust1 = 10;
	ui8Adjust2 = 254;
	ui8Adjust3 = 10;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);


	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);


	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);


	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
	PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, ui32Load);

	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);

	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);


	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust1 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_1);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust3 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust2 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);

	while(1)
	{
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00) {
			if (sw1Idle == 1) {
				sw1Press = 1;
				sw1Idle = 0;
			} else if (sw1Press == 1) {
				sw1Hold = sw1Hold + 1;
				sw1Press = 0;
			} else if (sw1Hold > 0 && sw1Hold < 25) {
				sw1Hold++;
			}
			if (sw1Hold >= 25) {
				sw1LongHold = 1;
				sw1Hold = 25;
			}
		} else {
			sw1Idle = 1;
			sw1Press = 0;
			sw1Hold = 0;
			sw1LongHold = 0;
		}
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00) {
			if (sw2Idle == 1) {
				sw2Press = 1;
				sw2Idle = 0;
			} else if (sw2Press == 1) {
				sw2Hold = sw2Hold + 1;
				sw2Press = 0;
			} else if (sw2Hold > 0 && sw2Hold < 25) {
				sw2Hold++;
			}
			if (sw2Hold >= 25) {
				sw2LongHold = 1;
				sw2Hold = 25;
			}
		} else {
			sw2Idle = 1;
			sw2Press = 0;
			sw2Hold = 0;
			sw2LongHold = 0;
			sw1PressCount = 0;
		}

		if (sw2LongHold == 1 && sw1Press == 1) {
			sw1PressCount++;
			if (sw1PressCount == 1) {
				manual_flag = 1;
				manual1 = 1;
				manual2 = 0;
				manual3 = 0;
				auto_flag = 0;
			} else if (sw1PressCount == 2) {
				manual_flag = 1;
				manual1 = 0;
				manual2 = 1;
				manual3 = 0;
				auto_flag = 0;
			} else {
				sw1PressCount = 0;
			}
		}
		if (sw2LongHold == 1 && sw1LongHold == 1) {
			manual_flag = 1;
			manual1 = 0;
			manual2 = 0;
			manual3 = 1;
			auto_flag = 0;
		}

		if (manual_flag) {
			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00) {
				if (manual1) {
					ui8Adjust1 = ui8Adjust1 + 1;
					if (ui8Adjust1 > 254)
					{
						ui8Adjust1 = 254;
					}
				} else if (manual2) {
					ui8Adjust2 = ui8Adjust2 + 1;
					if (ui8Adjust2 > 254)
					{
						ui8Adjust2 = 254;
					}
				} else if (manual3) {
					ui8Adjust3 = ui8Adjust3 + 1;
					if (ui8Adjust3 > 254)
					{
						ui8Adjust3 = 254;
					}
				}
			}
			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00) {
				if (manual1) {
					ui8Adjust1 = ui8Adjust1 - 1;
					if (ui8Adjust1 < 10)
					{
						ui8Adjust1 = 10;
					}
				} else if (manual2) {
					ui8Adjust2 = ui8Adjust2 - 1;
					if (ui8Adjust2 < 10)
					{
						ui8Adjust2 = 10;
					}
				} else if (manual3) {
					ui8Adjust3 = ui8Adjust3 - 1;
					if (ui8Adjust3 < 10)
					{
						ui8Adjust3 = 10;
					}
				}
			}
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust1 * ui32Load / 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust3 * ui32Load / 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust2 * ui32Load / 1000);

			SysCtlDelay(200000);
		}
		else if (auto_flag) {
			if (rg) {
				ui8Adjust1 = ui8Adjust1 - 1;
				ui8Adjust2 = ui8Adjust2 + 1;
			} else if (gb) {
				ui8Adjust2 = ui8Adjust2 - 1;
				ui8Adjust3 = ui8Adjust3 + 1;
			} else if (br) {
				ui8Adjust3 = ui8Adjust3 - 1;
				ui8Adjust1 = ui8Adjust1 + 1;
			} else {}
			if (ui8Adjust1 < 10)
			{
				ui8Adjust1 = 10;
			}
			if (ui8Adjust2 < 10)
			{
				ui8Adjust2 = 10;
			}
			if (ui8Adjust3 < 10)
			{
				ui8Adjust3 = 10;
			}
			if (ui8Adjust1 > 254)
			{
				ui8Adjust1 = 254;
				br = 0;
				rg = 1;
			}
			if (ui8Adjust2 > 254)
			{
				ui8Adjust2 = 254;
				rg = 0;
				gb = 1;
			}
			if (ui8Adjust3 > 254)
			{
				ui8Adjust3 = 254;
				gb = 0;
				br = 1;
			}
			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
			{
				speed--;
				if (speed < 1) speed = 1;
			}

			if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
			{
				speed++;
				if (speed > 100) speed = 100;
			}
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust1 * ui32Load / 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust3 * ui32Load / 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust2 * ui32Load / 1000);

			int delay = speed*10000;

			SysCtlDelay(delay);
		} else {
			manual_flag = 0;
			auto_flag = 0;
		}
	}

}

