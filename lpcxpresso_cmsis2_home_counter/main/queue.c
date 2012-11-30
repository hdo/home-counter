#include "LPC17xx.h"
#include "queue.h"
#include <stdlib.h>

uint8_t rbuffer[QUEUE_BUFFER_SIZE];
uint8_t rbuffer_head=0;
uint8_t rbuffer_tail=0;
uint8_t rbuffer_count=0;

void queue_reset() {
	rbuffer_head = 0;
	rbuffer_tail = 0;
	rbuffer_count = 0;
}

void queue_put(uint8_t data) {
	if (!queue_isFull()) {
		rbuffer[rbuffer_tail++] = data;
		rbuffer_count++;
		if (rbuffer_tail >= QUEUE_BUFFER_SIZE) {
			rbuffer_tail %= QUEUE_BUFFER_SIZE;
		}
	}
}

uint8_t queue_read() {
	if (rbuffer_count > 0) {
		uint8_t data = rbuffer[rbuffer_head++];
		rbuffer_count--;
		if (rbuffer_head >= QUEUE_BUFFER_SIZE) {
			rbuffer_head %= QUEUE_BUFFER_SIZE;
		}
		return data;
	}
	return 0;
}

uint8_t queue_isEmpty() {
	return rbuffer_count == 0;
}

uint8_t queue_isFull() {
	return rbuffer_count == QUEUE_BUFFER_SIZE;
}

uint8_t queue_dataAvailable() {
	return rbuffer_count > 0;
}

uint8_t queue_count() {
	return rbuffer_count;
}
