#include "utils.h"

// Define logging tag
static const char *TAG = "UTILS";

const char* get_frame_type_name(avb_frame_t type) 
{
    switch (type) {
        case avb_frame_gptp_announce: return "gPTP Announce";
        case avb_frame_gptp_sync: return "gPTP Sync";
        case avb_frame_gptp_follow_up: return "gPTP Follow Up";
        case avb_frame_gptp_pdelay_request: return "gPTP pDelay Request";
        case avb_frame_gptp_pdelay_response: return "gPTP pDelay Response";
        case avb_frame_gptp_pdelay_follow_up: return "gPTP pDelay Follow Up";
        case avb_frame_adp: return "ADP";
        case avb_frame_aecp_command_read: return "AECP Command Read";
        case avb_frame_aecp_response_read_entity: return "AECP Response Read Entity";
        case avb_frame_acmp: return "ACMP";
        case avb_frame_msrp_domain: return "MSRP Domain";
        case avb_frame_msrp_talker_advertise: return "MSRP Talker Advertise";
        case avb_frame_msrp_listener: return "MSRP Listener";
        case avb_frame_mvrp_vlan_identifier: return "MVRP VLAN Identifier";
        case avb_frame_avtp_stream: return "AVTP Stream";
        default: return "UNKNOWN";
    }
}

void binary_printf(int v)
{
    unsigned int mask=1<<((sizeof(int)<<3)-1);
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
}

