#ifndef COMPONENTS_ATDECC_INCLUDE_AVTP_H_
#define COMPONENTS_ATDECC_INCLUDE_AVTP_H_

#include <stdlib.h>
#include "esp_log.h"
#include "utils.h"

// AVTP subtypes and their values
typedef enum {
  avtp_subtype_aaf = 0x02,
  avtp_subtype_adp = 0xfa,
  avtp_subtype_aecp = 0xfb,
  avtp_subtype_acmp = 0xfc,
  avtp_subtype_maap = 0xfe
} avtp_subtype_t;

// MSRP attribute types and their values
typedef enum {
  msrp_attribute_type_talker_advertise = 0x01,
  msrp_attribute_type_listener = 0x03,
  msrp_attribute_type_domain = 0x04
} msrp_attribute_type_t;

avb_frame_type_t detect_avtp_frame_type(ethertype_t *ethertype, eth_frame_t *frame, ssize_t size);
void print_avtp_frame(avb_frame_type_t type, eth_frame_t *frame, ssize_t size);

#endif /* COMPONENTS_ATDECC_INCLUDE_AVTP_H_ */