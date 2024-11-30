#include "gptp.h"

// Define logging tag
static const char *TAG = "GPTP";

////////////
// gPTP messages
////////////

// Template gptp announce
static uint8_t frame_template_gptp_announce[] = {
    0x1b, // majorSdoId: gPTP domain (0x1), messageType: announce message (0xb)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x4c, // messageLength: 76
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x00,0x08, // flags: two-step: false, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x4b,0x5d, // sequenceID: 19293
    0x02, // controlField: follow_up message (2)
    0x00, // logMessagePeriod: 0 (1.000000s)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // reserved
    0x00,0x00, // originCurrentUTCOffset: 0
    0x00, // reserved
    0xf6, // priority1: 246
    0xf8, // grandmasterClockClass: 248
    0xfe, // grandmasterClockAccuracy: accuracy unknown (0xfe)
    0x00,0x01, // grandmasterClockVariance: 1
    0xf8, // priority2: 248
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // grandmasterClockIdentity
    0x00,0x00, // localStepsRemoved: 0
    0xa0, // timeSource: internal_oscillator (0xa0)
    0x00,0x08, // path trace tlv: path trace (0x0008)
    0x00,0x08, // path trace tlv: lengthField: 8
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // path trace tlv: pathSequence
}; // 76 bytes

// Template gptp sync
static uint8_t frame_template_gptp_sync[] = {
    0x10, // majorSdoId: gPTP domain (0x1), messageType: sync message (0x0)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x2c, // messageLength: 44
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x02,0x08, // flags: ptp_two_step: true, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x5a,0xe7, // sequenceID: 23271
    0x00, // controlField: sync message (0)
    0xfd, // logMessagePeriod: -3 (0.125000s)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // reserved
    0x00,0x00 // padding (not counted in messageLength)
}; // 44 bytes

// Template gptp follow up
static uint8_t frame_template_gptp_follow_up[] = {
    0x18, // majorSdoId: gPTP domain (0x1), messageType: follow up message (0x8)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x4c, // messageLength: 76
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x00,0x08, // flags: ptp_two_step: false, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x5a,0xe7, // sequenceID: 23271
    0x02, // controlField: follow_up message (2)
    0xfd, // logMessagePeriod: -3 (0.125000s)
    0x00,0x00,0x00,0x01,0x4b,0x60, // preciseOriginTimestamp (seconds): 84832
    0x35,0xd6,0xa2,0x98, // preciseOriginTimestamp (nanoseconds): 903258776
    0x00,0x03, // followup information tlv: tlvType: organization extension (0x0003)
    0x00,0x1c, // followup information tlv: lengthField: 28
    0x00,0x80,0xc2, // followup information tlv: organizationId: 32962
    0x00,0x00,0x01, // followup information tlv: organizationSubType: 1
    0x00,0x00,0x00,0x00, // followup information tlv: cumulativeScaledRateOffset: 0
    0x00,0x00, // followup information tlv: gmTimeBaseIndicator: 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // followup information tlv: lastGMPhaseChange
    0x00,0x00,0x00,0x00 // followup information tlv: scaledLastGmFreqChange: 0
}; // 76 bytes

// Template gptp peer delay request
static uint8_t frame_template_gptp_pdelay_request[] = {
    0x12, // majorSdoId: gPTP domain (0x1), messageType: peer_delay_request message (0x2)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x36, // messageLength: 54
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x00,0x08, // flags: ptp_two_step: false, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x4b,0x5d, // sequenceID: 19293
    0x05, // controlField: other message (5)
    0x00, // logMessagePeriod: 0 (1.000000s)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // reserved
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 // reserved
}; // 54 bytes

// Template gptp peer delay reponse
static uint8_t frame_template_gptp_pdelay_response[] = {
    0x13, // majorSdoId: gPTP domain (0x1), messageType: peer_delay_response message (0x3)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x36, // messageLength: 54
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x02,0x08, // flags: ptp_two_step: true, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x4b,0x3b, // sequenceID: 19259
    0x05, // controlField: other message (5)
    0x00, // logMessagePeriod: 0 (1.000000s)
    0x00,0x00,0x00,0x01,0x4b,0x61, // requestReceiptTimestamp (sec): 84833
    0x16,0xda,0x06,0x20, // requestReceiptTimestamp (nanosec): 383387168
    0x14,0x98,0x77,0x40,0xc7,0x88,0x00,0x02, // requestingSourcePortIdentity
    0x00,0x01 // requestingSourcePortId: 1
}; // 54 bytes

// Template gptp peer delay response follow up
static uint8_t frame_template_gptp_pdelay_response_follow_up[] = {
    0x1a, // majorSdoId: gPTP domain (0x1), messageType: peer_delay_response_follow_up message (0xa)
    0x02, // minorVersionPTP: 0, versionPTP: 2
    0x00,0x36, // messageLength: 54
    0x00, // domainNumber: 0
    0x00, // minorSdoId: 0
    0x00,0x08, // flags: ptp_two_step: false, ptp_timescale: true, ptp_utc_reasonable: false
    0x00,0x00,0x00,0x00,0x00,0x00, // correctionField: correctionNs: 0.000000ns
    0x00,0x00, // correctionField: correctionSubNs: 0
    0x00,0x00,0x00,0x00, // messageTypeSpecific: 0
    0x00,0x01,0xf2,0xff,0xfe,0x22,0x3b,0x14, // clockIdentity
    0x00,0x01, // sourcePortID: 1
    0x4b,0x3b, // sequenceID: 19259
    0x05, // controlField: other message (5)
    0x00, // logMessagePeriod: 0 (1.000000s)
    0x00,0x00,0x00,0x01,0x4b,0x61, // responseOriginTimestamp (sec): 84833
    0x16,0xda,0x67,0xb0, // responseOriginTimestamp (nanosec): 383412144
    0x14,0x98,0x77,0x40,0xc7,0x88,0x00,0x02, // requestingSourcePortIdentity
    0x00,0x01 // requestingSourcePortId: 1
}; // 54 bytes

/*
Return true if the candidate gm is better than the current gm
BMCAComparison Criteria:
	1.	Priority1:
	•	A user-configurable value (0-255).
	•	Lower values indicate higher priority.
	2.	Clock Class:
	•	Indicates the clock’s quality or type (e.g., GPS-synchronized clock, atomic clock).
	•	Lower values indicate higher quality.
	3.	Clock Accuracy:
	•	Indicates the clock’s estimated precision relative to the standard time.
	•	Smaller values represent higher accuracy.
	4.	Offset Scaled Log Variance:
	•	A measure of the clock’s stability and precision over time.
	•	Smaller values are better.
	5.	Priority2:
	•	Another user-configurable value, used as a secondary tiebreaker (0-255).
	•	Lower values indicate higher priority.
	6.	Clock Identity:
	•	A unique identifier for each clock (e.g., MAC address).
	•	Used as the final tiebreaker (lexicographically lowest wins).
*/
bool evaluate_bmca(gptp_gm_t *gm_candidate) {
    if (gm_candidate->priority1 < current_gm.priority1) {
        return true;
    }
    if (gm_candidate->clock_class < current_gm.clock_class) {
        return true;
    }
    if (gm_candidate->clock_accuracy < current_gm.clock_accuracy) {
        return true;
    }
    if (gm_candidate->clock_variance < current_gm.clock_variance) {
        return true;
    }
    if (gm_candidate->priority2 < current_gm.priority2) {
        return true;
    }
    if (gm_candidate->clock_id < current_gm.clock_id) {
        return true;
    }
    return false;
}

// Append gptpdu to a frame based on frame type and set the payload_size
// Prior to appending, overwrite template values with config data
void append_gptpdu(avb_frame_type_t type, eth_frame_t *frame) {
    switch (type) {
        case avb_frame_gptp_announce:
            memcpy(frame->payload, frame_template_gptp_announce, sizeof(frame_template_gptp_announce));
            frame->payload_size = sizeof(frame_template_gptp_announce);
            break;
        case avb_frame_gptp_sync:
            memcpy(frame->payload, frame_template_gptp_sync, sizeof(frame_template_gptp_sync));
            frame->payload_size = sizeof(frame_template_gptp_sync);
            break;
        case avb_frame_gptp_follow_up:
            memcpy(frame->payload, frame_template_gptp_follow_up, sizeof(frame_template_gptp_follow_up));
            frame->payload_size = sizeof(frame_template_gptp_follow_up);
            break;
        case avb_frame_gptp_pdelay_request:
            memcpy(frame->payload, frame_template_gptp_pdelay_request, sizeof(frame_template_gptp_pdelay_request));
            frame->payload_size = sizeof(frame_template_gptp_pdelay_request);
            break;
        case avb_frame_gptp_pdelay_response:
            memcpy(frame->payload, frame_template_gptp_pdelay_response, sizeof(frame_template_gptp_pdelay_response));
            frame->payload_size = sizeof(frame_template_gptp_pdelay_response);
            break;
        case avb_frame_gptp_pdelay_response_follow_up:
            memcpy(frame->payload, frame_template_gptp_pdelay_response_follow_up, sizeof(frame_template_gptp_pdelay_response_follow_up));
            frame->payload_size = sizeof(frame_template_gptp_pdelay_response_follow_up);
            break;
        default:
            ESP_LOGE(TAG, "Can't create %s, not supported yet.", get_frame_type_name(type));
        // Overwrite general gPTP config values
        memcpy(frame->payload + 20, CONFIG_CLOCK_ID, sizeof(CONFIG_CLOCK_ID)); // clock identity
    }
}

// Detect which kind of gPTP frame it is; assumes Ethertype is PTP
avb_frame_type_t detect_gptp_frame_type(eth_frame_t *frame, ssize_t size) {
    avb_frame_type_t frame_type = avb_frame_unknown;
    if (size <= ETH_HEADER_LEN) {
        ESP_LOGI(TAG, "Can't detect frame, too small: %d bytes", size);
    }
    else {
        uint8_t message_type;
        memcpy(&message_type, frame->payload, (1)); // 4 bits
        message_type = message_type & 0x0f; // mask out the left 4 bits
        switch(message_type) {
            case gptp_message_type_announce:
                frame_type = avb_frame_gptp_announce;
                break;
            case gptp_message_type_sync:
                frame_type = avb_frame_gptp_sync;
                break;
            case gptp_message_type_follow_up:
                frame_type = avb_frame_gptp_follow_up;
                break;
            case gptp_message_type_pdelay_request:
                frame_type = avb_frame_gptp_pdelay_request;
                break;
            case gptp_message_type_pdelay_response:
                frame_type = avb_frame_gptp_pdelay_response;
                break;
            case gptp_message_type_pdelay_response_follow_up:
                frame_type = avb_frame_gptp_pdelay_response_follow_up;
                break;
            default:
                ESP_LOGI(TAG, "Can't detect gPTP frame with unknown message type: 0x%x", message_type);
        }
    }
    return frame_type;
}

// Print out the frame details
void print_gptp_frame(avb_frame_type_t type, eth_frame_t *frame, int format) {
    
    ssize_t size = frame->payload_size + ETH_HEADER_LEN;
    if (frame->payload_size < 2) {
        ESP_LOGE(TAG, "Can't print frame, payload is too small: %d", frame->payload_size);
    }
    else {
        if (format == 0) { // short
            ESP_LOGI(TAG, "%s from %s", get_frame_type_name(type), mac_address_to_string(frame->header.src.addr));
        }
        else {
            ESP_LOGI(TAG, "*** Print Frame - %s (%d) ***", get_frame_type_name(type), size);
                    ESP_LOG_BUFFER_HEX("                        destination", &frame->header.dest, (6));
                    ESP_LOG_BUFFER_HEX("                             source", &frame->header.src, (6));
                    ESP_LOG_BUFFER_HEX("                          etherType", &frame->header.type, (2));
                    ESP_LOG_BUFFER_HEX("             majorSdoId,messageType", frame->payload, (1));
                    ESP_LOG_BUFFER_HEX("         minorVersionPtp,versionPtp", frame->payload + 1, (1));
                    ESP_LOG_BUFFER_HEX("                      messageLength", frame->payload + 2, (2));
                    ESP_LOG_BUFFER_HEX("                       domainNumber", frame->payload + 4, (1));
                    ESP_LOG_BUFFER_HEX("                         minorSdoId", frame->payload + 5, (1));
                    ESP_LOG_BUFFER_HEX("flags,ptpTimescale,ptpUtcReasonable", frame->payload + 6, (2));
                    ESP_LOG_BUFFER_HEX("       correctionField-correctionNs", frame->payload + 8, (6));
                    ESP_LOG_BUFFER_HEX("    correctionField-correctionSubNs", frame->payload + 14, (2));
                    ESP_LOG_BUFFER_HEX("                messageTypeSpecific", frame->payload + 16, (4));
                    ESP_LOG_BUFFER_HEX("                      clockIdentity", frame->payload + 20, (8));
                    ESP_LOG_BUFFER_HEX("                       sourcePortID", frame->payload + 28, (2));
                    ESP_LOG_BUFFER_HEX("                         sequenceID", frame->payload + 30, (2));
                    ESP_LOG_BUFFER_HEX("                       controlField", frame->payload + 32, (1));
                    ESP_LOG_BUFFER_HEX("                   logMessagePeriod", frame->payload + 33, (1));
            switch (type) {
                case avb_frame_gptp_announce:
                    ESP_LOG_BUFFER_HEX("                     reserved80bits", frame->payload + 34, (10));
                    ESP_LOG_BUFFER_HEX("             originCurrentUTCOffset", frame->payload + 44, (2));
                    ESP_LOG_BUFFER_HEX("                      reserved8bits", frame->payload + 46, (1));
                    ESP_LOG_BUFFER_HEX("                          priority1", frame->payload + 47, (1));
                    ESP_LOG_BUFFER_HEX("              grandmasterClockClass", frame->payload + 48, (1));
                    ESP_LOG_BUFFER_HEX("           grandmasterClockAccuracy", frame->payload + 49, (1));
                    ESP_LOG_BUFFER_HEX("           grandmasterClockVariance", frame->payload + 50, (2));
                    ESP_LOG_BUFFER_HEX("                          priority2", frame->payload + 52, (1));
                    ESP_LOG_BUFFER_HEX("           grandmasterClockIdentity", frame->payload + 53, (8));
                    ESP_LOG_BUFFER_HEX("                  localStepsRemoved", frame->payload + 61, (2));
                    ESP_LOG_BUFFER_HEX("                         timeSource", frame->payload + 63, (1));
                    ESP_LOG_BUFFER_HEX("                 tlvType(pathTrace)", frame->payload + 64, (2));
                    ESP_LOG_BUFFER_HEX("           pathTraceTlv-lengthField", frame->payload + 66, (2));
                    ESP_LOG_BUFFER_HEX("         pathTraceTlv(pathSequence)", frame->payload + 68, (8));
                    break;
                case avb_frame_gptp_sync:
                    ESP_LOG_BUFFER_HEX("                     reserved80bits", frame->payload + 34, (10));
                    break;
                case avb_frame_gptp_follow_up:
                    ESP_LOG_BUFFER_HEX("    preciseOriginTimestamp(seconds)", frame->payload + 34, (6));
                    ESP_LOG_BUFFER_HEX("preciseOriginTimestamp(nanoseconds)", frame->payload + 40, (4));
                    ESP_LOG_BUFFER_HEX("     tlvType(organizationExtension)", frame->payload + 44, (2));
                    ESP_LOG_BUFFER_HEX("                     tlvLengthField", frame->payload + 46, (2));
                    ESP_LOG_BUFFER_HEX("                 tlv-organizationId", frame->payload + 48, (3));
                    ESP_LOG_BUFFER_HEX("            tlv-organizationSubType", frame->payload + 51, (3));
                    ESP_LOG_BUFFER_HEX("     tlv-cumulativeScaledRateOffset", frame->payload + 54, (4));
                    ESP_LOG_BUFFER_HEX("            tlv-gmTimeBaseIndicator", frame->payload + 58, (2));
                    ESP_LOG_BUFFER_HEX("              tlv-lastGMPhaseChange", frame->payload + 70, (12));
                    ESP_LOG_BUFFER_HEX("         tlv-scaledLastGmFreqChange", frame->payload + 74, (4));
                    break;
                case avb_frame_gptp_pdelay_request:
                    ESP_LOG_BUFFER_HEX("                     reserved80bits", frame->payload + 34, (10));
                    ESP_LOG_BUFFER_HEX("                     reserved80bits", frame->payload + 44, (10));
                    break;
                case avb_frame_gptp_pdelay_response:
                    ESP_LOG_BUFFER_HEX("       requestReceiptTimestamp(sec)", frame->payload + 34, (6));
                    ESP_LOG_BUFFER_HEX("   requestReceiptTimestamp(nanosec)", frame->payload + 40, (4));
                    ESP_LOG_BUFFER_HEX("       requestingSourcePortIdentity", frame->payload + 44, (8));
                    ESP_LOG_BUFFER_HEX("             requestingSourcePortId", frame->payload + 52, (2));
                    break;
                case avb_frame_gptp_pdelay_response_follow_up:
                    ESP_LOG_BUFFER_HEX("       responseOriginTimestamp(sec)", frame->payload + 34, (6));
                    ESP_LOG_BUFFER_HEX("   responseOriginTimestamp(nanosec)", frame->payload + 40, (4));
                    ESP_LOG_BUFFER_HEX("       requestingSourcePortIdentity", frame->payload + 44, (8));
                    ESP_LOG_BUFFER_HEX("             requestingSourcePortId", frame->payload + 52, (2));
                    break;
                default:
                    ESP_LOGI(TAG, "Can't print frame with unknown Ethertype: 0x%x", type);
            }
            ESP_LOGI(TAG, "*** End of Frame ***");
        }
    }
}