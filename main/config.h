#ifndef ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_
#define ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_

// Configuration options for an AVB audio talker (single stream)
#define CONFIG_ENTITY_ID 0xf3c0401c5c114d2a
#define CONFIG_ENTITY_MODEL_ID 0x0
#define CONFIG_ASSOCIATION_ID 0x0
#define CONFIG_CLOCK_ID 0x0001a2b3c4d5e6f7
#define ENTITY_NAME "Simple Talker"
#define CONFIG_VENDOR_NAME "Scramble"
#define CONFIG_MODEL_NAME "ESP_AVB Simple Talker"
#define CONFIG_FIRMWARE_VERSION 1.0.0
#define CONFIG_GROUP_NAME ""
#define CONFIG_SERIAL_NUMBER 0
#define CONFIG_TALKER_GUID 0
#define CONFIG_STREAM_ID 0
#define CONFIG_PORT_ID 0x0001
#define CONFIG_AVB_INTERFACE_NAME "AVB Interface"
#define CONFIG_SAMPLE_RATE 48000
#define CONFIG_STREAM_FORMAT 0x0205021800406000 // AAF, 24/48, 32bit
#define CONFIG_GM_CAPABLE true // Will announce GM capability
#define CONFIG_GM_TIMEOUT 1000000 // Timeout for GM in usec
#define CONFIG_GM_LIST_SIZE 10 // Number of GMs to keep in list

//static const uint8_t CONFIG_CLOCK_ID[8] = { 0x00,0x01,0xa2,0xb3,0xc4,0xd5,0xe6,0xf7 };

// Declare the functions
void set_shared_value(int value);
int get_shared_value(void);

#endif /* ESP_AVB_SIMPLE_TALKER_INCLUDE_CONFIG_H_ */