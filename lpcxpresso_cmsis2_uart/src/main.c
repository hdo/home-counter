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
#include "sensors.h"
#include "ehz.h"
#include "leds.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>


extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];
extern volatile uint8_t ehz_value_parsed;
extern uint32_t ehz_value;
volatile uint32_t msTicks; // counter for 1ms SysTicks

//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
  msTicks++;
}

/*****************************************************************************
**   Main Function  main()
This program has been test on LPCXpresso 1700.
*****************************************************************************/
int main (void)
{

	// INIT SYSTICK
	// Setup SysTick Timer to interrupt at 1 msec intervals
	if (SysTick_Config(SystemCoreClock / 1000)) {
	    while (1);  // Capture error
	}

	led2_init();
	led2_on();

	init_sensors();
	add_ehz(0);

	const char* welcomeMsg = "\r\nUART2 Online:\r\n";
	//const char* welcomeMsg = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et e\r\n";
	//SystemInit();	//Called by startup code

	UARTInit(2, 9600);	/* baud rate setting */
	UARTSend(2, (uint8_t *)welcomeMsg , strlen(welcomeMsg) );
	led2_off();
	volatile int currentms = msTicks ;

	uint8_t state = 0;

	/* Loop forever */
	const char* msg = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores \r\n";
	while (1)
	{
		if (state == 0) {
			state++;
			uint32_t start = msTicks;
			UARTSend(2, (uint8_t *)msg , strlen(msg) );
			uint32_t stop = msTicks;
			uint32_t diff = stop - start;
			uint8_t puffer[20];
			uint8_t l = sprintf( puffer, "\r\nsend took %u ticks\r\n", diff );
			UARTSend(2, (uint8_t *)puffer, l );
			continue;
		}
		if (state == 1) {
			state++;
			uint32_t start = msTicks;
			logger_logString(msg);
			uint32_t stop = msTicks;
			uint32_t diff = stop - start;
			logger_logString("\r\nlog took ");
			logger_logNumber(diff);
			logger_logString(" ticks\r\n");
			continue;
		}
		if (state == 2) {
			state++;
			logger_logString("done");
			logger_logCRLF();
			continue;
		}

		if (logger_dataAvailable() && UART2TXReady()) {
			uint8_t data = logger_read();
			UARTSend2(data);
		}

		if ( UART2Count != 0 )
		{
			led2_on();
			currentms = msTicks;
			LPC_UART2->IER = IER_THRE | IER_RLS;				/* Disable RBR */

			int i = 0;
			for(; i < UART2Count; i++) {
				ehz_process_serial_data(UART2Buffer[i]);
			}
			UART2Count = 0;

			if (ehz_value_parsed > 0) {
				SENSOR_DATA* sd = get_sensor(SENSOR_TYPE_EHZ, 0);
				if (sd) {
					sd->value = ehz_value;
				}

				uint8_t puffer[20];
				uint8_t l = sprintf( puffer, "\n\rZaehlerstand: %u\n\r", ehz_value );
				UARTSend(2, (uint8_t *)puffer, l );
				ehz_value_parsed = 0;
			}

			LPC_UART2->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */
		}
		else {
			if (msTicks - currentms > 50) {
				led2_off();
			}
		}
	}
}

/*****************************************************************************
**                            End Of File
*****************************************************************************/
