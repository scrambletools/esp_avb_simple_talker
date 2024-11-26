#ifndef ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_
#define ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_

// Configuration options for an AVB audio talker (single stream)
#define CONFIG_ENTITY_ID 0x0
#define CONFIG_ENTITY_MODEL_ID 0x0
#define CONFIG_ASSOCIATION_ID 0x0
#define ENTITY_NAME "Simple Talker"
#define CONFIG_VENDOR_NAME "Scramble"
#define CONFIG_MODEL_NAME "ESP_AVB Simple Talker"
#define CONFIG_FIRMWARE_VERSION 1.0.0
#define CONFIG_GROUP_NAME ""
#define CONFIG_SERIAL_NUMBER 0
#define CONFIG_TALKER_GUID 0
#define CONFIG_STREAM_ID 0
#define CONFIG_AVB_INTERFACE_NAME "AVB Interface"
#define CONFIG_SAMPLE_RATE 48000
#define CONFIG_STREAM_FORMAT //{0x02,0x05,0x02,0x18,0x00,0x40,0x60,0x00} // AAF, 24/48, 32bit
#define CONFIG_CAN_BE_GM // Will announce GM capability

static const uint8_t CONFIG_CLOCK_ID[8] = { 0x00,0x01,0xa2,0xb3,0xc4,0xd5,0xe6,0xf7 };

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_ */