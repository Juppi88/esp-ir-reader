#ifndef __PROTOCOL_NEC_H
#define __PROTOCOL_NEC_H

#include "ir.h"

bool nec_decode(message_t *result, volatile uint32_t *buffer, volatile uint8_t buffer_length);

#endif
