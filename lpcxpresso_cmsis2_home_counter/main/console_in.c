#include "LPC17xx.h"
#include "console_in.h"
#include "queue.h"

uint8_t input_buffer_data[CONSOLE_IN_BUFFER_SIZE];
ringbuffer_t input_rbuffer = {.buffer=input_buffer_data, .head=0, .tail=0, .count=0, .size=CONSOLE_IN_BUFFER_SIZE};

void   console_in_reset() {
	queue_reset(&input_rbuffer);
}

void   console_in_put(uint8_t data) {
	queue_put(&input_rbuffer, data);
}

uint8_t console_in_isEmpty() {
	return queue_isEmpty(&input_rbuffer);
}

uint8_t console_in_isFull() {
	return queue_isFull(&input_rbuffer);
}

uint8_t console_in_read() {
	return queue_read(&input_rbuffer);
}

uint8_t console_in_dataAvailable() {
	return queue_dataAvailable(&input_rbuffer);
}

uint8_t console_in_count() {
	return queue_count(&input_rbuffer);
}
