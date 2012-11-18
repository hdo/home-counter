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
#include "version.h"
#include "s0_input.h"

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


/* wait */
uint32_t wait_ticks(uint32_t last_value, uint32_t ticks) {
	uint32_t ticks_now = clock_time();
	if (ticks_now >= last_value) {
		return (ticks_now - last_value) >= ticks;
	}
	else {
		// check for timer overflow
		return (UINT32_MAX - last_value + ticks_now) >= ticks;
	}
}

void delay_10ms(uint8_t ticks) {
	int currentms = clock_time() ;
	// wait 1s
	while(!(wait_ticks(currentms, ticks)));
}


/*--------------------------- main ---------------------------------*/

char ipstring [20];

extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];

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
	led_init();
	led2_on();
	led_all_on();
	delay_10ms(100);
	led_all_off();


	// sensor init
	led_on(0);
	init_sensors();
	add_ehz(0, "STROM                  ");
	add_s0(1,  "WASSER        [10L/Imp]");
	add_s0(2,  "GAS       [0.01m^3/Imp]");
	add_s0(3,  "WASSER GARTEN  [1L/Imp]");


	led_on(1);
	UARTInit(2, 9600);	/* baud rate setting */
	UARTInit(0, 115200);

	UARTSendCRLF(0);
	UARTSendCRLF(0);
	UARTSendStringln(0, "UART2 online ...");

	UARTSendString(0, PRODUCT_NAME);
	UARTSendString(0, " v");
	UARTSendNumber(0, VERSION_MAJOR);
	UARTSendString(0, ".");
	UARTSendNumber(0, VERSION_MINOR);
	UARTSendString(0, " BUILD ID ");
	UARTSendStringln(0, VERSION_BUILD_ID);
	UARTSendCRLF(0);

	logger_logStringln("log online ...");

	UARTSendString(0, "wait 500ms ...");
	delay_10ms(50);
	UARTSendStringln(0, " done");

	led_on(2);
	// ehz init
	UARTSendString(0, "init ehz ...");
	ehz_init();
	UARTSendStringln(0, " done");

	led_on(3);
	// ethernet init
	UARTSendString(0, "init ethernet ...");
	tapdev_init();
	UARTSendStringln(0, " done");

	led_on(4);
	UARTSendString(0, "init TCP/IP stack ...");
	// Initialize the uIP TCP/IP stack.
	uip_init();
	UARTSendStringln(0, " done");

	uip_ipaddr(ipaddr, MYIP_1,MYIP_2,MYIP_3,MYIP_4);
	uip_sethostaddr(ipaddr);	/* host IP address */
	uip_ipaddr(ipaddr, MYIP_1,MYIP_2,MYIP_3,1);
	uip_setdraddr(ipaddr);	/* router IP address */
	uip_ipaddr(ipaddr, 255,255,255,0);
	uip_setnetmask(ipaddr);	/* mask */

	led_on(5);
	UARTSendString(0, "init httpd ...");
	// Initialize the HTTP server, listen to port 80.
	httpd_init();
	UARTSendStringln(0, " done");

	delay_10ms(100);

	UARTSendStringln(0, "entering main loop ...");
	led2_off();
	led_all_off();


	uint32_t s0_msticks = 0;
	uint8_t s0_active = 0;
	uint32_t s0_state = 0;
	uint32_t s0_oldState = 0;
	uint32_t s0_newState = 0;

	/*
	 * INIT DEFAULT VALUES
	 */
	SENSOR_DATA* sd1 = get_sensor_by_id(1);
	if (sd1) {
		logger_logString("updating s0[1] value: ");
		sd1->value = 32998;
		logger_logNumberln(sd1->value);
	}

	sd1 = get_sensor_by_id(2);
	if (sd1) {
		logger_logString("updating s0[2] value: ");
		sd1->value = 375595;
		logger_logNumberln(sd1->value);
	}

	sd1 = get_sensor_by_id(3);
	if (sd1) {
		logger_logString("updating s0[3] value: ");
		sd1->value = 0;
		logger_logNumberln(sd1->value);
	}

	uint32_t msticks;

	while(1)
	{

		/* process logger */
		if (logger_dataAvailable() && UARTTXReady(0)) {
			uint8_t data = logger_read();
			UARTSendByte(0,data);
		}

		msticks = clock_time();

		/* process S0 input */
		process_s0(msticks);
		process_leds(msticks);

		if (s0_triggered(0)) {
			led_signal(1, 50, clock_time());
			SENSOR_DATA* sd = get_sensor_by_id(1);
			if (sd) {
				logger_logString("updating s0[1] value: ");
				sd->value++;
				logger_logNumberln(sd->value);
			}
		}

		if (s0_triggered(1)) {
			led_signal(2, 50, clock_time());
			SENSOR_DATA* sd = get_sensor_by_id(2);
			if (sd) {
				logger_logString("updating s0[2] value: ");
				sd->value++;
				logger_logNumberln(sd->value);
			}
		}

		if (s0_triggered(2)) {
			led_signal(3, 50, clock_time());
			SENSOR_DATA* sd = get_sensor_by_id(3);
			if (sd) {
				logger_logString("updating s0[3] value: ");
				sd->value++;
				logger_logNumberln(sd->value);
			}
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
			LPC_UART2->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */

			if (ehz_value_parsed() > 0) {
				led_signal(0, 30, clock_time());
				uint32_t ehz_value = ehz_get_value();
				uint32_t estimated_value = ehz_get_estimated_value();
				uint32_t parsing_errors = ehz_get_parsing_errors();

				SENSOR_DATA* sd = get_sensor_by_id(0);
				if (sd) {
					logger_logStringln("updating ehz[0] values ...");
					sd->value = ehz_value;
					sd->value2 = estimated_value;
					sd->errors = parsing_errors;
				}
			}

			if (ehz_parse_error()) {
				led_signal(5, 30, clock_time());
			}

		}
		else {
			led2_off();
		}
	}
}
