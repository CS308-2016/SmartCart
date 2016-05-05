
#include <stdio.h>
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

#define STATE_IDLE 1
#define STATE_PRESSED 2
#define STATE_HOLD 3
#define STATE_PAKAD_KE_BAITH_GAYA 4
#define STATE_RELEASED 5

#define MODE_AUTO 0
#define MODE_1 1
#define MODE_2 2
#define MODE_3 3

#define NO_LED   0
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

#define SWITCH_1 GPIO_PIN_4
#define SWITCH_2 GPIO_PIN_0

#define AUTO_STATE_RED  0
#define AUTO_STATE_GREEN  1
#define AUTO_STATE_BLUE  2

#define DELTA 100000
#define MAX_PAKAD_KE_BAITH_GAYA 100

void enablePins(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTF_BASE, RED_LED);
	GPIOPinTypePWM(GPIO_PORTF_BASE, BLUE_LED);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GREEN_LED);

	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE, SWITCH_1|SWITCH_2, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, SWITCH_1|SWITCH_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

int stateSwitch1 = STATE_IDLE;
int stateSwitch2 = STATE_IDLE;
int mode = MODE_AUTO;
int transitionMode = MODE_AUTO;
int autoState = AUTO_STATE_GREEN;
int delta = DELTA;

int pakadKeBaithGayaCounter1 = 0;
int pakadKeBaithGayaCounter2 = 0;

void increase(uint8_t* color) {
	if (*color == 255) {
		return;
	}
	*color = *color + 1;
}

void decrease(uint8_t* color) {
	if (*color == 1) {
		return;
	}
	*color = *color - 1;
}

void handleSwitchPress(int swich_pin, int* state, int* pakadKeBaithGayaCounter) {

	int current_state = *state;
	if(GPIOPinRead(GPIO_PORTF_BASE,swich_pin)==0x00)
	{
		if (current_state == STATE_IDLE) {
			*state = STATE_PRESSED;
		} else if (current_state == STATE_PRESSED) {
			*state = STATE_HOLD;
		} else if (current_state == STATE_HOLD) {
			*pakadKeBaithGayaCounter = *pakadKeBaithGayaCounter + 1;
		}

		if (current_state != STATE_PAKAD_KE_BAITH_GAYA &&
				*pakadKeBaithGayaCounter > MAX_PAKAD_KE_BAITH_GAYA) {
			*state = STATE_PAKAD_KE_BAITH_GAYA;
		}

	} else {
		if (current_state == STATE_PRESSED || current_state == STATE_HOLD || current_state == STATE_PAKAD_KE_BAITH_GAYA) {
			*state = STATE_RELEASED;
		} else if (current_state == STATE_RELEASED) {
			*state = STATE_IDLE;
		}

		*pakadKeBaithGayaCounter = 0;
	}
}

int main(void)
{
	volatile uint32_t ui32Load;
	volatile uint32_t ui32PWMClock;
	uint8_t redIntensity, greenIntensity, blueIntensity;
	redIntensity = 254;
	greenIntensity = 1;
	blueIntensity = 1;

	enablePins();

	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;

	PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, redIntensity * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, blueIntensity * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, greenIntensity * ui32Load / 1000);

	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);

	PWMGenEnable(PWM1_BASE, PWM_GEN_1);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);

	while(1)
	{
		if (mode == MODE_AUTO) {
			// Logic for Automode
			if (autoState == AUTO_STATE_GREEN) {
				if (redIntensity == 2) {
					autoState = AUTO_STATE_BLUE;
				}
				decrease(&redIntensity);
				increase(&greenIntensity);
			} else if (autoState == AUTO_STATE_BLUE) {
				if (greenIntensity == 2) {
					autoState = AUTO_STATE_RED;
				}
				decrease(&greenIntensity);
				increase(&blueIntensity);
			} else if (autoState == AUTO_STATE_RED) {
				if (blueIntensity == 2) {
					autoState = AUTO_STATE_GREEN;
				}
				decrease(&blueIntensity);
				increase(&redIntensity);
			}
		}

		handleSwitchPress(SWITCH_1, &stateSwitch1, &pakadKeBaithGayaCounter1);
		handleSwitchPress(SWITCH_2, &stateSwitch2, &pakadKeBaithGayaCounter2);

		// Handling the mode change
		if (stateSwitch1 == STATE_RELEASED && (stateSwitch2 == STATE_HOLD || stateSwitch2 == STATE_PAKAD_KE_BAITH_GAYA)) {
			if (transitionMode == MODE_AUTO) {
				transitionMode = MODE_1;
			} else if (transitionMode == MODE_1) {
				transitionMode = MODE_2;
			} else if (transitionMode == MODE_2) {
				transitionMode = MODE_3;
			}
		} else if (stateSwitch1 == STATE_PAKAD_KE_BAITH_GAYA && stateSwitch2 == STATE_PAKAD_KE_BAITH_GAYA) {
			transitionMode = MODE_3;
		} else if (stateSwitch1 == STATE_IDLE && stateSwitch2 == STATE_IDLE) {
			if (transitionMode != MODE_AUTO) {
				mode = transitionMode;
				delta = DELTA;
				transitionMode = MODE_AUTO;
			}
		} else if ((stateSwitch1 == STATE_PRESSED || stateSwitch1 == STATE_HOLD) && stateSwitch2 == STATE_IDLE) {
			if (mode == MODE_AUTO && delta > 10000 && stateSwitch1== STATE_PRESSED) {
				delta -= 10000;
			} else if (mode == MODE_1) {
				increase(&redIntensity);
			} else if (mode == MODE_2) {
				increase(&blueIntensity);
			} else if (mode == MODE_3) {
				increase(&greenIntensity);
			}
		} else if ((stateSwitch2 == STATE_PRESSED || stateSwitch2 == STATE_HOLD) && stateSwitch1 == STATE_IDLE) {
			if (mode == MODE_AUTO && stateSwitch2 == STATE_PRESSED) {
				delta += 10000;
			} else if (mode == MODE_1) {
				decrease(&redIntensity);
			} else if (mode == MODE_2) {
				decrease(&blueIntensity);
			} else if (mode == MODE_3) {
				decrease(&greenIntensity);
			}
		}

		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, redIntensity * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, blueIntensity * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, greenIntensity * ui32Load / 1000);
		SysCtlDelay(delta);
	}

}
