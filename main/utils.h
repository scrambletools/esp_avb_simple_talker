#ifndef COMPONENTS_ATDECC_INCLUDE_UTILS_H_
#define COMPONENTS_ATDECC_INCLUDE_UTILS_H_

#pragma once

#include "esp_log.h"
#include "lwip/prot/ethernet.h" // Ethernet header

#define AVTP_MAX_PAYLOAD_LENGTH 1486

// Basic ethernet frame
typedef struct {
    struct eth_hdr header;
    uint8_t payload[AVTP_MAX_PAYLOAD_LENGTH];
		ssize_t payload_size;
} eth_frame_t;

// Useful when doing things like printing frames
typedef enum {
	FrameTypeAdpdu = 0,
	FrameTypeAecpdu = 1,
	FrameTypeAcmpdu = 2
} FrameType;

const char* get_frame_type_name(FrameType type);
void binary_printf(int v);
void print_frame(FrameType type, eth_frame_t frame, ssize_t size);

#endif /* COMPONENTS_ATDECC_INCLUDE_UTILS_H_ */