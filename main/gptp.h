#ifndef ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_
#define ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_

#include <stdlib.h>
#include "esp_log.h"
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

// Managed grandmaster clock values with defaults
static uint8_t gm_clock_id[8] =       { 0 };
static uint8_t gm_priority1[1] =      { 246 };
static uint8_t gm_clock_class[1] =    { 248 };
static uint8_t gm_Clock_accuracy[1] = { 0xfe }; // unknown
static uint8_t gm_Clock_variance[1] = { 1 };
static uint8_t gm_priority2[1] =      { 248 };
static uint8_t gm_time_source[1] =    { 0xa0 }; // internal oscillator

// Managed state values
static struct timeval gptp_last_received_announce_timestamp; // received from remote gm
static struct timeval gptp_last_received_ntp_timestamp; // received from ntp server
static struct timeval gptp_last_sync_timestamp; // for follow up message
static struct timeval gptp_last_pdelay_response_timestamp; // for response follow up message
static int propogation_delay = 0; // nanoseconds
static bool localhost_is_gm = false; // local host is currently gm

// Managed sequence ids with initial values
static uint8_t seq_id_gptp_announce[2] =        { 0 };
static uint8_t seq_id_gptp_sync[2] =            { 0 }; // follow up uses same number
static uint8_t seq_id_gptp_pdelay_request[2] =  { 0 };
static uint8_t seq_id_gptp_pdelay_response[2] = { 0 }; // follow up uses same number

// Functions
void append_gptpdu(avb_frame_type_t type, eth_frame_t *frame);
avb_frame_type_t detect_gptp_frame_type(eth_frame_t *frame, ssize_t size);
void print_gptp_frame(avb_frame_type_t type, eth_frame_t *frame, int format);

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_ */