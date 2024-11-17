#ifndef COMPONENTS_ATDECC_INCLUDE_TALKER_H_
#define COMPONENTS_ATDECC_INCLUDE_TALKER_H_

#pragma once

#include "esp_vfs_l2tap.h"
#include "lwip/ip_addr.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include "esp_eth.h"
#include "esp_mac.h"
#include "lwip/prot/ethernet.h" // Ethernet header
#include "gptp.h"
#include "avtp.h"
#include "atdecc.h"
#include "utils.h"
#include "arpa/inet.h" // ntohs

// Constants
static const uint32_t ENTITY_ID = 0x001b92fffe01b930;  // Placeholder entity ID
static const uint64_t ENTITY_MODEL_ID = 0x123456789abcdef;  // Placeholder entity ID
static const uint16_t ENTITY_CAPABILITIES = 0x0000858a;
static const uint32_t INTERFACE_VERSION = 304;

// Global Ethernet handle
static esp_eth_handle_t eth_handle = NULL;

// Functions
static int init_l2tap_fd(int flags, uint16_t eth_type_filter);
static void eth_frame_logger(void *pvParameters);
esp_err_t initialize(esp_eth_handle_t eth_handle);
esp_err_t send_frame(eth_frame_t frame);
void send_entity_available();

#endif /* COMPONENTS_ATDECC_INCLUDE_TALKER_H_ */
