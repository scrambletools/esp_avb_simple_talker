#include "gptp.h"

// Define logging tag
static const char *TAG = "GPTP";

////////////
// gPTP messages
////////////

// Test gptp announce
static uint8_t gptp_announce[] = {
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

// Test gptp sync
static uint8_t gptp_sync[] = {
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

// Test gptp follow up
static uint8_t gptp_follow_up[] = {
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

// Test gptp peer delay request
static uint8_t gptp_pdelay_request[] = {
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

// Test gptp peer delay reponse
static uint8_t gptp_pdelay_response[] = {
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

// Test gptp peer delay response follow up
static uint8_t gptp_pdelay_reponse_follow_up[] = {
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