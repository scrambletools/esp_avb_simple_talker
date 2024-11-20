#ifndef COMPONENTS_ATDECC_INCLUDE_ATDECC_H_
#define COMPONENTS_ATDECC_INCLUDE_ATDECC_H_

#include <stdlib.h>
#include "esp_log.h"
#include "utils.h"
#include "avtp.h"

// ADP message types
typedef enum {
	entity_available,
	entity_departing,
	entity_discover
} adp_message_type_t;

// AECP message types (subset) and their values (1722.1 Clause 9)
typedef enum {
	aem_command = 0,
	aem_response = 1
} aecp_message_type_t;

// AECP command codes (subset) and their values (1722.1 Clause 7.4)
typedef enum {
	aecp_command_acquire_entity = 0x0000,
	aecp_command_lock_entity = 0x0001,
	aecp_command_entity_available = 0x0002,
	aecp_command_controller_available = 0x0003,
	aecp_command_read_descriptor = 0x0004
} aecp_command_codes_t;

// ACMP message types (subset) and their values (needs to add values)
typedef enum {
	connect_tx_command,
	connect_tx_response,
	disconnect_tx_command,
  disconnect_tx_response,
  connect_rx_response,
  disconnect_rx_response
} acmp_message_type_t;

// Functions
void append_adpdu(adp_message_type_t type, eth_frame_t *adpdu);
avb_frame_type_t detect_atdecc_frame_type(eth_frame_t *frame, ssize_t size);
void print_atdecc_frame(avb_frame_type_t type, eth_frame_t *frame, ssize_t size);

#endif /* COMPONENTS_ATDECC_INCLUDE_ATDECC_H_ */