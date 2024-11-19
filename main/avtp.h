#ifndef COMPONENTS_ATDECC_INCLUDE_AVTP_H_
#define COMPONENTS_ATDECC_INCLUDE_AVTP_H_

#include <stdlib.h>
#include "esp_log.h"
#include "utils.h"

void print_avtp_frame(avb_frame_t type, eth_frame_t *frame, ssize_t size);

#endif /* COMPONENTS_ATDECC_INCLUDE_AVTP_H_ */