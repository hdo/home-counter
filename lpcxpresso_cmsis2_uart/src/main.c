/***********************************************************************
 * $Id::                                                               $
 *
 * Project:	uart: Simple UART echo for LPCXpresso 1700
 * File:	uarttest.c
 * Description:
 * 			LPCXpresso Baseboard uses pins mapped to UART3 for
 * 			its USB-to-UART bridge. This application simply echos
 * 			all characters received.
 *
 ***********************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **********************************************************************/

/*****************************************************************************
 *   History
 *   2010.07.01  ver 1.01    Added support for UART3, tested on LPCXpresso 1700
 *   2009.05.27  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "LPC17xx.h"
#include "type.h"
#include "uart.h"
#include "leds.h"
#include <stdio.h>
#include <string.h>

#define SERIAL_BUFFER_SIZE 12
#define SEARCH_PATTERN_LENGTH 6
#define EHZ_VALUE_LENGTH 11

extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];
volatile uint32_t msTicks; // counter for 1ms SysTicks

/* we're looking for pattern  "1*255(" */
const uint8_t search_pattern[SEARCH_PATTERN_LENGTH] = {0x31,0x2A,0x32,0x35,0x35,0x28};
uint8_t search_match = 0;
uint8_t serialbuffer[SERIAL_BUFFER_SIZE];
uint8_t serialbuffer_index = 0;
uint32_t ehz_value = 0;
uint8_t ehz_value_parsed = 0;

//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
  msTicks++;
}

void process_serial_data(uint8_t data) {
	UARTSend2(data);
	if (search_match >= SEARCH_PATTERN_LENGTH) {


		// here comes the data
		if (serialbuffer_index >= SERIAL_BUFFER_SIZE) {
			// error (should not happen)
			serialbuffer_index = 0;
			search_match = 0;
		}
		serialbuffer[serialbuffer_index++] = data;
		if (serialbuffer_index >= EHZ_VALUE_LENGTH || data == ')') {
			UARTSend2('#');

			// we're expecting 11 bytes of data
			// * parse data here *
			uint8_t i = 0;
			uint8_t d;
			// atoi conversion, ignoring non-digits
			ehz_value = 0;
			for (;i<serialbuffer_index;i++) {
				d = serialbuffer[i];
				if (d >= '0' && d <= '9') {
					//UARTSend2(d);
					d -= '0';
					ehz_value *= 10;
					ehz_value += d;
				}
			}

			// reset buffer
			serialbuffer_index = 0;
			search_match = 0;
			ehz_value_parsed = 1;
		}
	}
	else {
		if (data == search_pattern[search_match]) {
			search_match++;
		}
		else {
			search_match = 0;
		}
		UARTSend2('0' + search_match);
	}
}

/*****************************************************************************
**   Main Function  main()
This program has been test on LPCXpresso 1700.
*****************************************************************************/
int main (void)
{

	// INIT SYSTICK
	// Setup SysTick Timer to interrupt at 10 msec intervals
	if (SysTick_Config(SystemCoreClock / 100)) {
	    while (1);  // Capture error
	}

	led2_init();
	led2_on();
	const char* welcomeMsg = "UART3 Online:\r\n";
	//SystemInit();	//Called by startup code

	UARTInit(2, 9600);	/* baud rate setting */
	UARTSend(2, (uint8_t *)welcomeMsg , strlen(welcomeMsg) );
	led2_off();
	volatile int currentms = msTicks ;


	/* Loop forever */
	while (1)
	{
		if ( UART2Count != 0 )
		{
			led2_on();
			currentms = msTicks;
			LPC_UART2->IER = IER_THRE | IER_RLS;				/* Disable RBR */

			int i = 0;
			for(; i < UART2Count; i++) {
				process_serial_data(UART2Buffer[i]);
			}


			if (ehz_value_parsed > 0) {
				uint8_t puffer[20];
				uint8_t l = sprintf( puffer, "\n\rZaehlerstand: %u\n\r", ehz_value );
				UARTSend(2, (uint8_t *)puffer, l );
				ehz_value_parsed = 0;
			}


			UART2Count = 0;
			LPC_UART2->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */
		}
		else {
			if (msTicks - currentms > 5) {
				led2_off();
			}
		}
	}
}

/*****************************************************************************
**                            End Of File
*****************************************************************************/
