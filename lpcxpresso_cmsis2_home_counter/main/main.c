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

// ************************************
// Define initial values for S0 counter
// ************************************

// WASSER
#define INITIAL_VALUE_SO_0 33280
// GAS
#define INITIAL_VALUE_SO_1 379762
// WASSER GARTEN
#define INITIAL_VALUE_SO_2 27
// TEST
#define INITIAL_VALUE_SO_3 0




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
#include "rtc.h"
#include "eeprom_utils.h"

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

// RTC
#define EEPROM_IDENTIFIER_OFFSET 0
#define EEPROM_BOOT_COUNTER_OFFSET 4
#define EEPROM_S0_VALUE_OFFSET 10
#define PERSISTENCE_IDENTIFIER 47110815



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


/* ----------------------- s0 persistence -------------------- */

void set_s0_values(uint32_t values[]) {
	uint8_t i;
	SENSOR_DATA* sd_elem;
	for(i=0; i < S0_INPUT_COUNT; i++) {
		sd_elem = get_sensor_by_id(i+1);
		if (sd_elem) {
			UARTSendString(0, "set value of s0 with id ");
			UARTSendNumber(0, i+1);
			UARTSendString(0, " : ");
			sd_elem->value = values[i];
			UARTSendNumberln(0, sd_elem->value);
		}
	}
}

void save_s0_values(void) {
	uint8_t i;
	SENSOR_DATA* sd_elem;
	for(i=0; i < S0_INPUT_COUNT; i++) {
		// s0 sensors start at 1
		sd_elem = get_sensor_by_id(i+1);
		if (sd_elem) {
			// OFFSET OF EEPROM BEGINS AT 10
			uint16_t addr = (i+EEPROM_S0_VALUE_OFFSET)*4;
			eeprom_check_update_uint32(addr, sd_elem->value);
		}
	}
}


void restore_s0_values(void) {
	uint8_t i;
	uint32_t values[] = {0, 0, 0, 0};
	UARTSendStringln(0, "restoring s0 values from eeprom ...");
	for(i=0; i < S0_INPUT_COUNT; i++) {
		uint16_t addr = (i+EEPROM_S0_VALUE_OFFSET)*4;
		values[i] = eeprom_get_uint32(addr);
	}
	set_s0_values(values);
	UARTSendStringln(0, " done");
}


/*--------------------------- main ---------------------------------*/

char ipstring [20];

extern volatile uint32_t UART2Count;
extern volatile uint8_t UART2Buffer[BUFSIZE];

extern uint32_t boot_up_counter;

// RTC
extern volatile uint32_t rtc_alarm_secs, rtc_alarm_minutes, rtc_alarm_hours;
volatile uint32_t uptime_secs = 0;

int main(void)
{
	uint32_t i;
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

    /* rtc init */
	RTCInit();
	LPC_RTC->CIIR = IMSEC;
	RTCStart();
	NVIC_EnableIRQ(RTC_IRQn);




	// led init
	led_init();
	led2_on();
	led_all_on();
	delay_10ms(50);
	led_all_off();


	// sensor init
	led_on(0);
	init_sensors();
	add_ehz(0, "STROM");
	add_s0(1,  "WASSER");
	add_s0(2,  "GAS");
	add_s0(3,  "WASSER GARTEN");
	add_s0(4,  "TEST");


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

	/*
	 *
	 * TODO: when to load values from eeprom
	 */

	UARTSendString(0, "identifier: ");
	UARTSendNumberln(0, eeprom_get_uint32(EEPROM_IDENTIFIER_OFFSET));

	if (eeprom_get_uint32(EEPROM_IDENTIFIER_OFFSET) == PERSISTENCE_IDENTIFIER) {
		UARTSendStringln(0, "updating boot up counter ... ");
		// process boot up counter
		boot_up_counter = eeprom_get_uint32(EEPROM_BOOT_COUNTER_OFFSET);
		boot_up_counter++;
		eeprom_set_uint32(EEPROM_BOOT_COUNTER_OFFSET, boot_up_counter);

		UARTSendString(0, "boot up counter: ");
		UARTSendNumberln(0, boot_up_counter);

		UARTSendStringln(0, "loading initial values from eeprom ... ");
		restore_s0_values();
	}
	else {
		UARTSendStringln(0, "WARNING: eeprom is not initalized! ");
		UARTSendString(0, "initializing eeprom ... ");
		eeprom_set_uint32(EEPROM_IDENTIFIER_OFFSET, PERSISTENCE_IDENTIFIER);
		eeprom_set_uint32(EEPROM_BOOT_COUNTER_OFFSET, 1);
		UARTSendStringln(0, "done");
	}


	// load initial values via backup register of rtc
	// check whether RTC battery is present
	UARTSendStringln(0, "loading initial values from RTC ...");
	UARTSendString(0, "RTC identifier value: ");
	UARTSendNumberln(0, LPC_RTC->GPREG4);

	if (LPC_RTC->GPREG4 == PERSISTENCE_IDENTIFIER) {
		UARTSendStringln(0, "found values in RTC registers ...");
		// values are already in RTC registers
	}
	else {
		UARTSendStringln(0, "WARNING: RTC registers are not initialized!");
		UARTSendStringln(0, "initializing RTC registers ...");
		// init backup register
		LPC_RTC->GPREG4 = PERSISTENCE_IDENTIFIER;
		LPC_RTC->GPREG0 = INITIAL_VALUE_SO_0;
		LPC_RTC->GPREG1 = INITIAL_VALUE_SO_1;
		LPC_RTC->GPREG2 = INITIAL_VALUE_SO_2;
		LPC_RTC->GPREG3 = INITIAL_VALUE_SO_3;
	}

	uint32_t init_values[] = {0, 0, 0, 0};
	init_values[0] = LPC_RTC->GPREG0;
	init_values[1] = LPC_RTC->GPREG1;
	init_values[2] = LPC_RTC->GPREG2;
	init_values[3] = LPC_RTC->GPREG3;
	set_s0_values(init_values);

	UARTSendString(0, "saving values to eeprom ... ");
	save_s0_values();
	UARTSendStringln(0, " done");


	logger_logStringln("log online ...");

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


	uint32_t s0_msticks = 0;
	uint8_t s0_active = 0;
	uint32_t s0_state = 0;
	uint32_t s0_oldState = 0;
	uint32_t s0_newState = 0;
	uint32_t msticks;
	SENSOR_DATA* sd_elem;

	UARTSendString(0, "loading initial s0 values ...");


	UARTSendStringln(0, " done");
	UARTSendStringln(0, "entering main loop ...");
	led2_off();
	led_all_off();

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
			led_signal(0, 50, clock_time());
			sd_elem = get_sensor_by_id(1);
			if (sd_elem) {
				logger_logString("updating s0[1] value: ");
				sd_elem->value++;
				logger_logNumberln(sd_elem->value);
				// update backup register
				LPC_RTC->GPREG0 = sd_elem->value;
			}
		}

		if (s0_triggered(1)) {
			led_signal(1, 50, clock_time());
			sd_elem = get_sensor_by_id(2);
			if (sd_elem) {
				logger_logString("updating s0[2] value: ");
				sd_elem->value++;
				logger_logNumberln(sd_elem->value);
				// update backup register
				LPC_RTC->GPREG1 = sd_elem->value;
			}
		}

		if (s0_triggered(2)) {
			led_signal(2, 50, clock_time());
			sd_elem = get_sensor_by_id(3);
			if (sd_elem) {
				logger_logString("updating s0[3] value: ");
				sd_elem->value++;
				logger_logNumberln(sd_elem->value);
				// update backup register
				LPC_RTC->GPREG2 = sd_elem->value;
			}
		}

		if (s0_triggered(3)) {
			led_signal(3, 50, clock_time());
			sd_elem = get_sensor_by_id(4);
			if (sd_elem) {
				logger_logString("updating s0[4] value: ");
				sd_elem->value++;
				logger_logNumberln(sd_elem->value);
				// update backup register
				LPC_RTC->GPREG3 = sd_elem->value;
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
				led_signal(4, 30, clock_time());
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

		if (rtc_alarm_secs != 0) {
			rtc_alarm_secs = 0;
			uptime_secs++;
		}

		if (rtc_alarm_minutes != 0) {
			rtc_alarm_minutes = 0;
		}

		if (rtc_alarm_hours != 0) {
			rtc_alarm_hours = 0;
			logger_logString("saving values to eeprom ...");
			save_s0_values();
			logger_logStringln(" done");
		}
	}
}
