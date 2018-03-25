#include "ets_sys.h"
#include "gpio.h"
#include "os_type.h"
#include "osapi.h"
#include "ir.h"
#include "protocol/nec.h"

// --------------------------------------------------------------------------------

static void ICACHE_FLASH_ATTR ir_timeout(void *arg);

// --------------------------------------------------------------------------------

void ICACHE_FLASH_ATTR ir_initialize(ir_reader_t *sensor, uint8_t gpio_pin, uint8_t flags)
{
	// Store the GPIO pin number used by the sensor.
	sensor->gpio_pin = gpio_pin;

	// Set sensor state to idle.
	sensor->state = IR_STATE_IDLE;
	sensor->flags = flags;
	sensor->buffer_offset = 0;
	sensor->interrupt_time = 0;

	// Setup a timer which determines when the transmission ends.
	os_timer_setfn(&sensor->timer, (os_timer_func_t *)ir_timeout, sensor);

	// Set the GPIO pin for read mode.
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(sensor->gpio_pin));

	// Attach a handler method for interrupts, unless the library user has a global handler elsewhere.
	if (flags & IR_FLAG_INTERRUPTS) {
		ETS_GPIO_INTR_ATTACH(ir_handle_interrupt, sensor);
	}

	// Enable interrupts for the sensor pin.
	gpio_pin_intr_state_set(GPIO_ID_PIN(sensor->gpio_pin), GPIO_PIN_INTR_ANYEDGE);

	// GPIO interrupts must be enabled for this library to work.
	ETS_GPIO_INTR_ENABLE();
}

bool ICACHE_FLASH_ATTR ir_decode(ir_reader_t *sensor, message_t *message)
{
	// No transmission has happened, which means there is no value to decode.
	if (sensor->state != IR_STATE_STOPPED) {
		return false;
	}

	bool succeeded = false;

	// Currently only NEC protocol is supported.
	if (nec_decode(message, sensor->buffer, sensor->buffer_offset)) {
		succeeded = true;
	}

	// Reset state to allow a new packet to be received.
	sensor->state = IR_STATE_IDLE;
	sensor->buffer_offset = 0;

	return succeeded;
}

void ir_handle_interrupt(void *arg)
{
	ir_reader_t *sensor = (ir_reader_t *)arg;

	// Clear interrupt status.
    uint32_t status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);

	// If the signal has stopped, do not process the interrupt.
	if (sensor->state == IR_STATE_STOPPED) {
		return;
	}

	uint32_t time = system_get_time();

	// Start receiving a packet over IR.
	// If already receiving, measure the interval from the previous interrupt.
	if (sensor->state == IR_STATE_IDLE) {
		sensor->state = IR_STATE_RECEIVING;
	}
	else if (sensor->buffer_offset < IR_BUFFER_SIZE) {
		sensor->buffer[sensor->buffer_offset++] = (time - sensor->interrupt_time);
	}

	// Store start time for the next interrupt.
	sensor->interrupt_time = time;

	// Rearm the timeout timer.
	os_timer_disarm(&sensor->timer);
	os_timer_arm(&sensor->timer, 15, 0);
}

static void ICACHE_FLASH_ATTR ir_timeout(void *arg)
{
	ETS_GPIO_INTR_DISABLE();

	// If something has been written into the buffer, stop processing until the data has been decoded.
	ir_reader_t *sensor = (ir_reader_t *)arg;
    sensor->state = (sensor->buffer_offset != 0 ? IR_STATE_STOPPED : IR_STATE_IDLE);

	ETS_GPIO_INTR_ENABLE();
}
