#ifndef __IR_H
#define __IR_H

#include "os_type.h"

// --------------------------------------------------------------------------------

// Sensor initialization flags
typedef enum {
	// Attach sensor GPIO interrupts automatically.
	// If not used, ir_handle_interrupt must be called manually.
	IR_FLAG_INTERRUPTS = 0x1,
} ir_flags_t;

// IR sensor state
typedef enum {
	IR_STATE_IDLE,		// Waiting for an interrupt
	IR_STATE_RECEIVING,	// Currently receiving data
	IR_STATE_STOPPED	// Receiving stopped, ready to decode data
} ir_state_t;

// Decoded IR remote message
typedef struct {
	uint8_t address;	// Device address
	uint8_t command;	// Received command
	uint32_t raw_data;	// Raw decoded data
} message_t;

// Receiving buffer for incoming data (i.e. pulse intervals).
#define IR_BUFFER_SIZE 100

// --------------------------------------------------------------------------------

typedef struct {
	uint8_t gpio_pin;	// GPIO pin the sensor is connected to
	uint8_t	flags;		// Flags this sensor was initialized with

	os_timer_t timer;	// Timer for determining when a transmission has ended

	volatile ir_state_t state;	// Current state of the sensor
	volatile uint32_t interrupt_time;			// Timestamp of the most recent interrupt
	volatile uint32_t buffer[IR_BUFFER_SIZE];	// Read buffer
	volatile uint8_t buffer_offset;
} ir_reader_t;

// --------------------------------------------------------------------------------

// Call this to setup the IR sensor.
void ICACHE_FLASH_ATTR ir_initialize(ir_reader_t *sensor, uint8_t gpio_pin, uint8_t flags);

// Poll this regularly to see whether the sensor has received any data.
// When the function returns true, 'message' will contain the decoced message.
bool ICACHE_FLASH_ATTR ir_decode(ir_reader_t *sensor, message_t *message);

// Call this interrupt handler manually if (and only if) a global handler is used outside this library.
// The argument is the ir_reader_t struct which triggered the interrupt.
void ir_handle_interrupt(void *arg);

#endif
