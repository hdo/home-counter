/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: main.c,v 1.16 2006/06/11 21:55:03 adam Exp $
 *
 */

// *********************************************************************
// Define the IP address to be used for the MCU running the TCP/IP stack
// *********************************************************************
#define MYIP_1	192
#define MYIP_2	168
#define MYIP_3	2
#define MYIP_4	200


#include "LPC17xx.h"

#include "timer.h"
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "type.h"
#include "uart.h"
#include "sensors.h"
#include "ehz.h"
#include "leds.h"
#include "clock-arch.h"
#include "logger.h"

#include <cr_section_macros.h>
#include <NXP/crp.h>
// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

#if defined ( _RDB1768_)
// Used to decide if "First connection message" is printed to LCD
int firstconnection = 0;
#endif


#define BUF ((struct uip_eth_hdr *)&uip_buf[0])


/*--------------------------- uip_log ---------------------------------*/

void uip_log(char *m)
{
  //printf("uIP log message: %s\n", m);
}


/*--------------------------- main ---------------------------------*/

char ipstring [20];

extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];
extern volatile uint8_t ehz_value_parsed;
extern uint32_t ehz_value;
uint32_t old_ehz_value = 0;
uint32_t last_ehz_value = 0;
uint32_t last_ehz_msTicks = 0;




int main(void)
{
	unsigned int i;
	uip_ipaddr_t ipaddr;	/* local IP address */
	struct timer periodic_timer, arp_timer;

	// Code Red - if CMSIS is being used, then SystemInit() routine
	// will be called by startup code rather than in application's main()
#ifndef __USE_CMSIS
	// system init
	SystemInit();                                      /* setup core clocks */
#endif

	// clock init
	clock_init();
		// two timers for tcp/ip
	timer_set(&periodic_timer, CLOCK_SECOND / 2); /* 0.5s */
	timer_set(&arp_timer, CLOCK_SECOND * 10);	/* 10s */

	// led init
	led2_init();
	led2_on();

	// sensor init
	init_sensors();
	add_ehz(0);


	UARTInit(2, 9600);	/* baud rate setting */
	UARTSendStringln(2, "UART2 online ...");
	logger_logStringln("log online ...");

	volatile int currentms = clock_time() ;
	UARTSendString(2, "wait 3s ...");
	while(clock_time() - currentms < 300);
	UARTSendStringln(2, " done");

	UARTSendString(2, "init ethernet ...");
	
	// ethernet init
	tapdev_init();

	UARTSendStringln(2, " done");

	UARTSendString(2, "init TCP/IP stack ...");
	// Initialize the uIP TCP/IP stack.
	uip_init();
	UARTSendStringln(2, " done");

	uip_ipaddr(ipaddr, MYIP_1,MYIP_2,MYIP_3,MYIP_4);
	uip_sethostaddr(ipaddr);	/* host IP address */
	uip_ipaddr(ipaddr, MYIP_1,MYIP_2,MYIP_3,1);
	uip_setdraddr(ipaddr);	/* router IP address */
	uip_ipaddr(ipaddr, 255,255,255,0);
	uip_setnetmask(ipaddr);	/* mask */

	UARTSendString(2, "init httpd ...");
	// Initialize the HTTP server, listen to port 80.
	httpd_init();
	UARTSendStringln(2, " done");

	UARTSendStringln(2, "entering main loop ...");
	led2_off();
	while(1)
	{

		/* process logger */
		if (logger_dataAvailable() && UARTTXReady(2)) {
			uint8_t data = logger_read();
			UARTSendByte(2,data);
		}


 	    /* receive packet and put in uip_buf */
		uip_len = tapdev_read(uip_buf);
    	if(uip_len > 0)		/* received packet */
    	{ 
      		if(BUF->type == htons(UIP_ETHTYPE_IP))	/* IP packet */
      		{
	      		uip_arp_ipin();	
	      		uip_input();
	      		/* If the above function invocation resulted in data that
	         		should be sent out on the network, the global variable
	         		uip_len is set to a value > 0. */
 
	      		if(uip_len > 0)
        		{
#if defined ( _RDB1768_)
	      			if (firstconnection == 0) {
	      				firstconnection = 1;
	      			}
#endif
	      			uip_arp_out();
	        		tapdev_send(uip_buf,uip_len);
	      		}
      		}
	      	else if(BUF->type == htons(UIP_ETHTYPE_ARP))	/*ARP packet */
	      	{
	        	uip_arp_arpin();
		      	/* If the above function invocation resulted in data that
		         	should be sent out on the network, the global variable
		         	uip_len is set to a value > 0. */
		      	if(uip_len > 0)
	        	{
		        	tapdev_send(uip_buf,uip_len);	/* ARP ack*/
		      	}
	      	}
    	}
    	else if(timer_expired(&periodic_timer))	/* no packet but periodic_timer time out (0.5s)*/
    	{
      		timer_reset(&periodic_timer);
	  
      		for(i = 0; i < UIP_CONNS; i++)
      		{
      			uip_periodic(i);
		        /* If the above function invocation resulted in data that
		           should be sent out on the network, the global variable
		           uip_len is set to a value > 0. */
		        if(uip_len > 0)
		        {
		          uip_arp_out();
		          tapdev_send(uip_buf,uip_len);
		        }
      		}
	     	/* Call the ARP timer function every 10 seconds. */
			if(timer_expired(&arp_timer))
			{
				timer_reset(&arp_timer);
				uip_arp_timer();
			}
    	}

		if ( UART2Count != 0 ) {
			led2_on();
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
			led2_off();
		}
	}
}
