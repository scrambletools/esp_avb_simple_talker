#ifndef COMPONENTS_ATDECC_INCLUDE_UTILS_H_
#define COMPONENTS_ATDECC_INCLUDE_UTILS_H_

#include "esp_log.h"
#include "lwip/prot/ethernet.h" // Ethernet header
#include <string.h>

#define AVTP_MAX_PAYLOAD_LENGTH 1486
#define ETH_HEADER_LEN 14

// Basic ethernet frame
typedef struct {
    struct eth_hdr header;
    uint8_t payload[AVTP_MAX_PAYLOAD_LENGTH];
		ssize_t payload_size;
} eth_frame_t;

// Ethertypes
typedef enum {
	ethertype_msrp = 0x22ea,
	ethertype_avtp = 0x22f0,
	ethertype_mvrp = 0x88f5,
	ethertype_gptp = 0x88f7
} ethertype_t;

// Useful when doing things like printing frames
typedef enum {
	avb_frame_gptp_announce,
	avb_frame_gptp_sync,
	avb_frame_gptp_follow_up,
	avb_frame_gptp_pdelay_request,
	avb_frame_gptp_pdelay_response,
	avb_frame_gptp_pdelay_follow_up,
	avb_frame_adp, // all adp frames have the same format
	avb_frame_aecp_acquire_entity, // command and response have same format
	avb_frame_aecp_lock_entity, // command and response have same format
	avb_frame_aecp_command_entity_available,
	avb_frame_aecp_response_entity_available,
	avb_frame_aecp_controller_available, // command and response have same format
	avb_frame_aecp_command_read_descriptor, // all read descriptor commands have same format
	avb_frame_aecp_response_read_entity,
	avb_frame_acmp, // all acmp frames have the same format
	avb_frame_avtp_stream,
	avb_frame_maap_announce,
	avb_frame_msrp_domain,
	avb_frame_msrp_talker_advertise,
	avb_frame_msrp_listener,
	avb_frame_mvrp_vlan_identifier,
	avb_frame_unknown
} avb_frame_type_t;

// Functions
const char* get_frame_type_name(avb_frame_type_t type);
void binary_printf(int v);

#endif /* COMPONENTS_ATDECC_INCLUDE_UTILS_H_ */