#include "LPC17xx.h"
#include "console.h"
#include "console_out.h"
#include <stdlib.h>

uint8_t console_enabled = 0;

/**
 * expected zero terminated string
 */
void console_printString(char* data) {
	while(!console_out_isFull() && *data) {
		console_printByte(*data++);
	}
}

void console_printStringln(char* data) {
	console_printString(data);
	console_printCRLF();
}
void console_printNumber(uint32_t value) {
	char buf[10];
	itoa(value, buf, 10);
	console_printString((char*) buf);
}

void console_printNumberln(uint32_t value) {
	console_printNumber(value);
	console_printCRLF();
}

void console_printCRLF() {
	console_printByte(13);
	console_printByte(10);
}

void console_printByte(uint8_t data) {
	if (console_enabled) {
		console_out_put(data);
	}
}

void console_setEnabled(uint8_t enabled) {
	console_enabled = enabled;
}
