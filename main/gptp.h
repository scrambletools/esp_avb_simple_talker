#ifndef COMPONENTS_ATDECC_INCLUDE_GPTP_H_
#define COMPONENTS_ATDECC_INCLUDE_GPTP_H_

#include <stdlib.h>
#include "esp_log.h"
#include "utils.h"

// gPTP message types and their values
typedef enum {
  gptp_message_type_announce = 0xb,
	gptp_message_type_sync = 0x0,
	gptp_message_type_follow_up = 0x8,
	gptp_message_type_pdelay_request = 0x2,
	gptp_message_type_pdelay_response = 0x3,
	gptp_message_type_pdelay_follow_up = 0xa
} gptp_message_type_t;

avb_frame_type_t detect_gptp_frame_type(eth_frame_t *frame, ssize_t size);
void print_gptp_frame(avb_frame_type_t type, eth_frame_t *frame, ssize_t size);

#endif /* COMPONENTS_ATDECC_INCLUDE_GPTP_H_ */