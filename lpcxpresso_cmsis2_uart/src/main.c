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
#include <string.h>

extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];
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
			UARTSend(2, (uint8_t *)UART2Buffer, UART2Count );
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
