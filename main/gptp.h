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
    uint8_t clock_id[8];
    uint8_t clock_class[1];
    uint8_t clock_accuracy[1];
    uint8_t clock_variance[1];
		uint8_t priority1[1];
    uint8_t priority2[1];
		uint8_t time_source[1];
} gptp_gm_t;

// Managed state values
static int mean_link_delay = 0; // nanoseconds
static bool gm_is_present = false; // a gm is present (local or remote)
static bool gm_is_local = false; // local host is currently gm

// Managed grandmaster data with defaults
// Taken from remote announce or internal config if local host is gm
static gptp_gm_t current_gm = {
	{ 0 },    // clock_id
	{ 248 },  // clock_class
	{ 0xfe }, // clock_accuracy (unknown)
	{ 1 },    // clock_variance
	{ 246 },  // priority1
	{ 248 },  // priority2
	{ 0xa0 }  // time_source (internal oscillator)
};

// Managed timestamps
static struct timeval gptp_timestamp_last_received_sync; // sync transmit ts from remote gm
static struct timeval gptp_timestamp_last_received_pdelay_request; // request receipt ts by local
static struct timeval gptp_timestamp_last_received_pdelay_response; // request receipt ts by peer
static struct timeval gptp_timestamp_last_received_pdelay_response_follow_up; // response origin timestamp
static struct timeval gptp_timestamp_last_sent_sync; // sync transmit ts from local gm
static struct timeval gptp_timestamp_last_sent_pdelay_response; // response transmit ts from local

// Managed sequence ids with initial values
static uint8_t gptp_sequence_id_last_received_sync[2] =            { 0 };
static uint8_t gptp_sequence_id_last_received_pdelay_request[2] =  { 0 };
static uint8_t gptp_sequence_id_last_sent_sync[2] =                { 0 };
static uint8_t gptp_sequence_id_last_sent_pdelay_request[2] =      { 0 };

// Functions
bool evaluate_bmca(gptp_gm_t *gm_candidate);
void append_gptpdu(avb_frame_type_t type, eth_frame_t *frame);
avb_frame_type_t detect_gptp_frame_type(eth_frame_t *frame, ssize_t size);
void print_gptp_frame(avb_frame_type_t type, eth_frame_t *frame, int format);

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_GPTP_H_ */