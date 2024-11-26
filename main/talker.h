#ifndef ESP_AVB_SIMPLE_TALKER_INCLUDE_TALKER_H_
#define ESP_AVB_SIMPLE_TALKER_INCLUDE_TALKER_H_

#include "lwip/ip_addr.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include "esp_eth.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_vfs_l2tap.h"
#include "lwip/prot/ethernet.h" // Ethernet header
#include "freertos/queue.h"
#include "arpa/inet.h" // ntohs
#include "gptp.h"
#include "avtp.h"
#include "atdecc.h"
#include "utils.h"
#include "config.h"

// for l2tap
#define INVALID_FD     -1
#define ETH_INTERFACE "ETH_DEF"
#define PRINT_FRAME_FORMAT 0 // 0=short, 1=long

// Functions
static void gptp_task(void *pvParameters);
void send_gptp_pdelay_response(eth_frame_t * req_frame);
static void avtp_task(void *pvParameters);
static void msrp_task(void *pvParameters);
static void mvrp_task(void *pvParameters);
int init_l2tap_fd(int flags, ethertype_t ethertype);
esp_err_t init_all_l2tap_fds();
void close_all_l2tap_fds();
esp_err_t get_frame(int fd, ethertype_t ethertype, eth_frame_t *frame);
void set_frame_header(uint8_t *dest_addr, ethertype_t ethertype, eth_frame_t *frame);
esp_err_t send_frame(eth_frame_t *frame);
void print_frame(avb_frame_type_t type, eth_frame_t *frame, int format);
void start_talker(esp_netif_iodriver_handle handle);
void stop_talker();

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_TALKER_H_ */
