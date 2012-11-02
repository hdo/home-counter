#include "LPC17xx.h"
#include "leds.h"

uint8_t led_array[] = {LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7};

// Function to initialise GPIO to access led
void led_init (void)
{
	// Set P0_22 to 00 - GPIO
	LPC_PINCON->PINSEL1	&= (~(3 << 12));
	// Set GPIO - P0_22 - to be output
	LPC_GPIO0->FIODIR |= (1 << LED_ONBOARD);

	// set P2_0 - P2_5 to 00 - GPIO (see UM page 110)
	LPC_PINCON->PINSEL4	&= (~(3 << 0));
	LPC_PINCON->PINSEL4	&= (~(3 << 2));
	LPC_PINCON->PINSEL4	&= (~(3 << 4));
	LPC_PINCON->PINSEL4	&= (~(3 << 6));
	LPC_PINCON->PINSEL4	&= (~(3 << 8));
	LPC_PINCON->PINSEL4	&= (~(3 << 10));
	LPC_PINCON->PINSEL4	&= (~(3 << 12));
	LPC_PINCON->PINSEL4	&= (~(3 << 14));

	// Set GPIO - P2_0 - P2_5 - to be output
	LPC_GPIO2->FIODIR |= (1 << LED0) | (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4) | (1 << LED5) | (1 << LED6) | (1 << LED7);

}


// Function to turn led on
void led2_on (void)
{
	LPC_GPIO0->FIOSET = (1 << LED_ONBOARD);
}

// Function to turn led off
void led2_off (void)
{
	LPC_GPIO0->FIOCLR = (1 << LED_ONBOARD);
}

// Function to invert current state of led
void led2_invert (void)
{
	int ledstate;

	// Read current state of GPIO P0_0..31, which includes led
	ledstate = LPC_GPIO0->FIOPIN;
	// Turn off led if it is on
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO0->FIOCLR = ledstate & (1 << LED_ONBOARD);
	// Turn on led if it is off
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO0->FIOSET = ((~ledstate) & (1 << LED_ONBOARD));
}

// Function to turn led on
void led_on (uint8_t channel)
{
	if (channel < MAX_LEDS) {
		LPC_GPIO2->FIOSET = (1 << led_array[channel]);
	}
}

// Function to turn led off
void led_off (uint8_t channel)
{
	if (channel < MAX_LEDS) {
        LPC_GPIO2->FIOCLR = (1 << led_array[channel]);
	}
}



// Function to invert current state of led
void led_invert (uint8_t channel)
{
	if (channel < MAX_LEDS) {
		int ledstate;

		// Read current state of GPIO P0_0..31, which includes led
		ledstate = LPC_GPIO2->FIOPIN;

		LPC_GPIO2->FIOCLR = ledstate & (1 << led_array[channel]);
		LPC_GPIO2->FIOSET = ((~ledstate) & (1 << led_array[channel]));
	}
}

void led_all_on (void)
{
	uint8_t i;
	for(i=0; i < sizeof(led_array); i++) {
		led_on(i);
	}
}

// Function to turn led off
void led_all_off (void)
{
	uint8_t i;
	for(i=0; i < sizeof(led_array); i++) {
		led_off(i);
	}
}



// Function to invert current state of led
void led_all_invert (void)
{
	uint8_t i;
	for(i=0; i < sizeof(led_array); i++) {
		led_invert(i);
	}
}
