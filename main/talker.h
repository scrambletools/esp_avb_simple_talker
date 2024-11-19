#ifndef COMPONENTS_ATDECC_INCLUDE_TALKER_H_
#define COMPONENTS_ATDECC_INCLUDE_TALKER_H_

#include "lwip/ip_addr.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include "esp_eth.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_vfs_l2tap.h"
#include "lwip/prot/ethernet.h" // Ethernet header
#include "gptp.h"
#include "avtp.h"
#include "atdecc.h"
#include "utils.h"
#include "arpa/inet.h" // ntohs

// for l2tap
#define INVALID_FD     -1
#define ETH_INTERFACE "ETH_DEF"

// Functions
int init_l2tap_fd(int flags, ethertype_t ethertype);
int init_l2tap_fd_for_sending(ethertype_t ethertype);
void start_talker(esp_netif_iodriver_handle handle);
static void frame_watcher_task(void *pvParameters);
esp_err_t init_all_l2tap_fds();
esp_err_t send_frame(eth_frame_t *frame);
void send_entity_available();

#endif /* COMPONENTS_ATDECC_INCLUDE_TALKER_H_ */
