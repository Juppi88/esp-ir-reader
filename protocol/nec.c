#include "nec.h"

// --------------------------------------------------------------------------------

// Macros for decoding NEC protocol data.

#define MATCHES(value, target, error) \
	(value >= target - error && value <= target + error)

#define IS_LEADER_LONG(value) \
	MATCHES(value, 9000, 2250)

#define IS_LEADER_SHORT(value) \
	MATCHES(value, 4500, 1125)

#define IS_SHORT_PULSE(value) \
	MATCHES(value, 562, 400)

#define IS_LONG_PULSE(value) \
	MATCHES(value, 1688, 422)

// --------------------------------------------------------------------------------

bool nec_decode(message_t *message, volatile uint32_t *buffer, volatile uint8_t buffer_length)
{
	// Make sure the data buffer has enough values to decode a 32bit value.
	// 2 intervals for the leading pulse and 2*32 intervals for the data are needed.
	if (buffer_length < 2 + 2 * 32) {
		return false;
	}

	// Make sure the buffer starts with NEC protocol leader (long 9ms pulse + short 4.5ms pulse).
	if (!IS_LEADER_LONG(buffer[0]) ||
		!IS_LEADER_SHORT(buffer[1])) {
		return false;
	}

	uint32_t value = 0, i = 0;

	// Decode the received 32-bit message.
	while (i < 32) {

		int index = 2 + 2 * i;

		if (IS_SHORT_PULSE(buffer[index]) && IS_LONG_PULSE(buffer[index + 1])) {

			// Short pulse and long space equals a one bit.
			value |= (1 << i);
		}
		else if (IS_SHORT_PULSE(buffer[index]) && IS_SHORT_PULSE(buffer[index + 1])) {

			// Short pulse and short space equals a zero bit.
			value &= ~(1 << i);
		}
		else {
			// ERROR! Neither 0 or 1, which probably means a read error occured somewhere.
			return false;
		}

		i++;
	}

	// The message is a 32bit value, where the first byte is device address, second one it's
	// logical inverse, third the command and fourth its logical inverse.

	// Get the logical inverses which we can use to verify the received data.
	uint8_t address_verification = ~(uint8_t)((value >> 8) & 0xFF);
	uint8_t command_verification = ~(uint8_t)((value >> 24) & 0xFF);

	// Get the actual data.
	message->address = (uint8_t)(value & 0xFF);
	message->command = (uint8_t)((value >> 16) & 0xFF);
	message->raw_data = value;

	// If the values and their inverses match, the message was valid.
	return (message->address == address_verification &&
			message->command == command_verification);
}
