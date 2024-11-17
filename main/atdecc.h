#ifndef COMPONENTS_ATDECC_INCLUDE_ATDECC_H_
#define COMPONENTS_ATDECC_INCLUDE_ATDECC_H_

#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "utils.h"

typedef enum {
	entity_available = 0,
	entity_departing = 1
} adpdu_t;

typedef enum {
	read_entity_response = 0,
	acquire_entity_response = 1,
  lock_entity_response = 2,
  entity_available_reponse = 3
} aecpdu_t;

typedef enum {
	connect_tx_command = 0,
	connect_tx_response = 1,
	disconnect_tx_command = 2,
  disconnect_tx_response = 3,
  connect_rx_response = 4,
  disconnect_rx_response = 5
} acmpdu_t;

void append_adpdu(adpdu_t type, eth_frame_t *adpdu);

#endif /* COMPONENTS_ATDECC_INCLUDE_ATDECC_H_ */