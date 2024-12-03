#ifndef ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_
#define ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_

#include <stdlib.h>
#include <esp_log.h>
#include "utils.h"
#include "config.h"

// gPTP message types and their values
typedef enum {
	gptp_message_type_sync = 0x0,
	gptp_message_type_pdelay_request = 0x2,
	gptp_message_type_pdelay_response = 0x3,
	gptp_message_type_follow_up = 0x8,
	gptp_message_type_pdelay_response_follow_up = 0xa,
	gptp_message_type_announce = 0xb
} gptp_message_type_t;

// Grandmaster structure
typedef struct {
	uint16_t port_id;
	uint8_t priority_1;
    uint8_t clock_class;
    uint8_t clock_accuracy;
    uint16_t clock_variance;
	uint8_t priority_2;
	uint64_t clock_id;
	uint16_t steps_removed;
	uint8_t time_source;
	struct timeval last_sync;
} gptp_gm_info_t;

// Peer delay information structure
typedef struct {
	uint16_t sequence_id;
    struct timeval timestamp_request_transmission; // from local
	struct timeval timestamp_request_receipt; // by peer
    struct timeval timestamp_response_transmission; // from peer
	struct timeval timestamp_response_receipt; // by local
	uint64_t calculated_pdelay; // in nsec; ((req_r - req_t) + (res_r - res_t)) / 2
} gptp_pdelay_info_t;

// Time offset information structure
typedef struct {
	uint16_t sequence_id;
    struct timeval timestamp_sync_transmission; // from master
	struct timeval timestamp_sync_receipt; // by local
	struct timeval timestamp_request_transmission; // from local
	struct timeval timestamp_request_receipt; // by peer
	uint64_t calculated_offset; // in nsec; ((sync_r - sync_t) - (req_r - req_t)) / 2
} gptp_offset_info_t;

// Functions
void get_gm_is_present(bool *is_present);
void get_gm_is_local(bool *is_local);
void get_time_is_set(bool *is_set);
void get_current_gm(gptp_gm_info_t *gm_info);
void get_current_pdelay(gptp_pdelay_info_t *pdelay_info);
void get_current_offset(gptp_offset_info_t *offset_info);
void get_gm_list(gptp_gm_info_t *gm_list);
bool evaluate_bmca(gptp_gm_info_t *gm_candidate_info);
uint64_t calculate_pdelay(gptp_pdelay_info_t *pdelay_info);
uint64_t calculate_offset(gptp_offset_info_t *offset_info);
void append_gptpdu(avb_frame_type_t type, eth_frame_t *frame);
avb_frame_type_t detect_gptp_frame_type(eth_frame_t *frame, ssize_t size);
void print_gptp_frame(avb_frame_type_t type, eth_frame_t *frame, int format);

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_ */