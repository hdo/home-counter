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



extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];
extern volatile uint8_t ehz_value_parsed;
extern uint32_t ehz_value;
volatile uint32_t msTicks; // counter for 1ms SysTicks
uint32_t old_ehz_value = 0;
uint32_t last_ehz_value = 0;
uint32_t last_ehz_msTicks = 0;


//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
  msTicks++;
}


uint32_t clock_time() {
	return msTicks;
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

	init_sensors();
	add_ehz(0);

	//SystemInit();	//Called by startup code

	UARTInit(2, 9600);	/* baud rate setting */

	//const char* welcomeMsg = "\r\nUART2 Online:\r\n";
	//UARTSend(2, (uint8_t *)welcomeMsg , strlen(welcomeMsg) );
	UARTSendStringln(2, "UART2 online ...");
	logger_logStringln("logger online ...");
	led2_off();
	volatile int currentms = msTicks ;
	UARTSendStringln(2, "wait 3s ...");
	while(msTicks - currentms < 300);
	UARTSendStringln(2, "done ...");

	/* Loop forever */
	while (1)
	{

		if (logger_dataAvailable() && UARTTXReady(2)) {
			uint8_t data = logger_read();
			UARTSendByte(2, data);
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
				if (old_ehz_value == 0) {
					old_ehz_value = ehz_value;
				}
				if (ehz_value >= old_ehz_value) {
					old_ehz_value = ehz_value;
					uint32_t current_msTicks = clock_time(); // one tick equals 10ms see lpc17xx_systick.h
					SENSOR_DATA* sd = get_sensor(SENSOR_TYPE_EHZ, 0);
					if (sd) {
						sd->value = ehz_value;
						logger_logString("main: ehz value: ");
						logger_logNumberln(ehz_value);

						if (last_ehz_value == 0) {
							last_ehz_value = ehz_value;
							last_ehz_msTicks = current_msTicks;
						}
						else {
							uint32_t diff = 0;
							if (current_msTicks > last_ehz_msTicks) {
								diff = current_msTicks - last_ehz_msTicks;
							}
							else {
								// check for timer overflow
								diff = UINT32_MAX - last_ehz_msTicks + current_msTicks;
							}
							logger_logString("diff ticks: ");
							logger_logNumberln(diff);
							uint32_t diffv = ehz_value - last_ehz_value;
							logger_logString("diff value: ");
							logger_logNumberln(diffv);
							// 10 seconds
							if (diff > 1000) {
								uint32_t estimated = diffv * 360000 / diff;

								/* update value */
								sd->value2 = estimated;

								logger_logString("main: estimated ehz value: ");
								logger_logNumberln(estimated);

								last_ehz_msTicks = current_msTicks;
								last_ehz_value = ehz_value;
							}
						}
					}
				}
				else {
					// log error
					// value is expected to be greater than previous value
					SENSOR_DATA* sd = get_sensor(SENSOR_TYPE_EHZ, 0);
					if (sd) {
						sd->errors++;
						logger_logString("main: error count: ");
						logger_logNumberln(sd->errors);
					}
					logger_logString("main: unexpected ehz value: ");
					logger_logNumber(ehz_value);
					logger_logString(" old value: ");
					logger_logNumberln(old_ehz_value);
				}
				ehz_value_parsed = 0;
			}

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
