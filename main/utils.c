#include "utils.h"

// Define logging tag
static const char *TAG = "UTILS";

// Get the name of the Ethertype
const char* get_ethertype_name(ethertype_t ethertype) 
{
    switch (ethertype) {
        case ethertype_gptp: return "gPTP";
        case ethertype_avtp: return "AVTP";
        case ethertype_msrp: return "MSRP";
        case ethertype_mvrp: return "MVRP";
        default: return "Unknown";
    }
}

// Get the name of the frame type
const char* get_frame_type_name(avb_frame_type_t type) 
{
    switch (type) {
        case avb_frame_gptp_announce: return "gPTP Announce";
        case avb_frame_gptp_sync: return "gPTP Sync";
        case avb_frame_gptp_follow_up: return "gPTP Follow Up";
        case avb_frame_gptp_pdelay_request: return "gPTP pDelay Request";
        case avb_frame_gptp_pdelay_response: return "gPTP pDelay Response";
        case avb_frame_gptp_pdelay_response_follow_up: return "gPTP pDelay Response Follow Up";
        case avb_frame_adp_entity_available: return "ADP Entity Available";
        case avb_frame_adp_entity_departing: return "AD Entity Departing";
        case avb_frame_adp_entity_discover: return "ADP Entity Discover";
        case avb_frame_aecp_command_acquire_entity: return "AECP Command Acquire Entity";
        case avb_frame_aecp_response_acquire_entity: return "AECP Response Acquire Entity";
	    case avb_frame_aecp_command_lock_entity: return "AECP Command Lock Entity";
        case avb_frame_aecp_response_lock_entity: return "AECP Response Lock Entity";
	    case avb_frame_aecp_command_entity_available: return "AECP Command Entity Available";
	    case avb_frame_aecp_response_entity_available: return "AECP Response Entity Available";
	    case avb_frame_aecp_command_controller_available: return "AECP Command Controller Available";
        case avb_frame_aecp_response_controller_available: return "AECP Response Controller Available";
        case avb_frame_aecp_command_read_descriptor: return "AECP Command Read Descriptor";
        case avb_frame_aecp_response_read_entity: return "AECP Response Read Entity";
        case avb_frame_acmp_connect_tx_command: return "ACMP Connect TX Command";
	    case avb_frame_acmp_connect_tx_response: return "ACMP Connect TX Response";
	    case avb_frame_acmp_disconnect_tx_command: return "ACMP Disconnect TX Command";
        case avb_frame_acmp_disconnect_tx_response: return "ACMP Disconnect TX Response";
	    case avb_frame_acmp_get_tx_state_command: return "ACMP Get TX State Command";
	    case avb_frame_acmp_get_tx_state_response: return "ACMP Get TX State Response";
	    case avb_frame_acmp_connect_rx_command: return "ACMP Connect RX Command";
        case avb_frame_acmp_connect_rx_response: return "ACMP Connect RX Response";
        case avb_frame_acmp_disconnect_rx_command: return "ACMP Disconnect RX Command";
	    case avb_frame_acmp_disconnect_rx_response: return "ACMP Disconnect RX Response";
	    case avb_frame_acmp_get_rx_state_command: return "ACMP Get RX State Command";
	    case avb_frame_acmp_get_rx_state_response: return "ACMP Get RX State Response";
        case avb_frame_avtp_stream: return "AVTP Stream";
        case avb_frame_maap_announce: return "MAAP Announce";
        case avb_frame_msrp_talker_advertise: return "MSRP Talker Advertise";
        case avb_frame_msrp_talker_failed: return "MSRP Talker Failed";
        case avb_frame_msrp_listener: return "MSRP Listener";
        case avb_frame_msrp_domain: return "MSRP Domain";
        case avb_frame_mvrp_vlan_identifier: return "MVRP VLAN Identifier";
        default: return "Unknown";
    }
}

// Convert octet buffer to uint64_t or uint32_t; assumes big-endian buffer
uint64_t octets_to_uint(const uint8_t *buffer, size_t size, int return_64bit) {
    if (buffer == NULL || size == 0 || size > 8) {
        return 0; // Return 0 for invalid input
    }

    uint64_t result = 0;

    // Combine bytes into a uint64_t
    for (size_t i = 0; i < size; i++) {
        result |= ((uint64_t)buffer[i] << (8 * (size - 1 - i))); // Big-endian order
    }

    // If the caller requests a 32-bit integer, truncate the result to 32 bits
    if (!return_64bit) {
        return (uint32_t)result;
    }

    // Otherwise, return as a 64-bit integer
    return result;
}

// Helper function for 32-bit
uint64_t octets_to_uint64(const uint8_t *buffer, size_t size) {
    return octets_to_uint(buffer, size, true);
}

// Helper function for 32-bit
uint32_t octets_to_uint32(const uint8_t *buffer, size_t size) {
    return octets_to_uint(buffer, size, false);
}

// Reverses the order of octets in a buffer
void reverse_octets(uint8_t *buffer, size_t size) {
    if (buffer == NULL || size <= 1) {
        return; // Nothing to do for NULL or single-item buffers
    }
    for (size_t i = 0; i < size / 2; i++) {
        // Swap items at index i and (size - 1 - i)
        uint8_t temp = buffer[i];
        buffer[i] = buffer[size - 1 - i];
        buffer[size - 1 - i] = temp;
    }
}

// Generates a string of bits from a uint8_t buffer
void octets_to_binary_string(const uint8_t *buffer, size_t size, char *bit_string) {
    if (buffer == NULL || bit_string == NULL || size == 0) {
        return; // Handle invalid input gracefully
    }
    // Convert each byte to its binary representation
    char *ptr = bit_string;
    for (size_t i = 0; i < size; i++) {
        for (int bit = 7; bit >= 0; bit--) { // MSB to LSB
            *ptr++ = (buffer[i] & (1 << bit)) ? '1' : '0';
        }
    }
    // Null-terminate the string
    *ptr = '\0';
}

// Generates a string of bits fron a uint64_t; num_bits can be 64,32,16 or 8
void int_to_binary_string(uint64_t value, int num_bits, char *bit_string, bool reverse_order) {
    if (bit_string == NULL || num_bits <= 0 || num_bits > 64) {
        return; // Handle invalid input gracefully
    }

    // Generate the binary string
    for (int i = 0; i < num_bits; i++) {
        if (!reverse_order) {
            // Big-endian: MSB first
            bit_string[i] = (value & (1ULL << (num_bits - 1 - i))) ? '1' : '0';
        } else {
            // Little-endian: LSB first
            bit_string[i] = (value & (1ULL << i)) ? '1' : '0';
        }
    }
    // Null-terminate the string
    bit_string[num_bits] = '\0';
}

// Helper function for uint64_t
void int_to_binary_string_64(uint64_t value, char *bit_string) {
    int_to_binary_string(value, 64, bit_string, false);
}

// Helper function for uint32_t
void int_to_binary_string_32(uint32_t value, char *bit_string) {
    int_to_binary_string(value, 32, bit_string, false);
}

// Helper function for uint16_t
void int_to_binary_string_16(uint16_t value, char *bit_string) {
    int_to_binary_string(value, 16, bit_string, false);
}

// Reverse endianness of a uint64_t; num_bits can be 64,32,16 or 8
uint64_t reverse_endianness(uint64_t value, int num_bits) {
    switch (num_bits) {
        case 16:
            return ((value & 0x00FF) << 8) |  // Move byte 0 to byte 1
                   ((value & 0xFF00) >> 8);  // Move byte 1 to byte 0
        case 32:
            return ((value & 0x000000FF) << 24) |  // Move byte 0 to byte 3
                   ((value & 0x0000FF00) << 8)  |  // Move byte 1 to byte 2
                   ((value & 0x00FF0000) >> 8)  |  // Move byte 2 to byte 1
                   ((value & 0xFF000000) >> 24);   // Move byte 3 to byte 0
        case 64:
            return ((value & 0x00000000000000FFULL) << 56) |  // Move byte 0 to byte 7
                   ((value & 0x000000000000FF00ULL) << 40) |  // Move byte 1 to byte 6
                   ((value & 0x0000000000FF0000ULL) << 24) |  // Move byte 2 to byte 5
                   ((value & 0x00000000FF000000ULL) << 8)  |  // Move byte 3 to byte 4
                   ((value & 0x000000FF00000000ULL) >> 8)  |  // Move byte 4 to byte 3
                   ((value & 0x0000FF0000000000ULL) >> 24) |  // Move byte 5 to byte 2
                   ((value & 0x00FF000000000000ULL) >> 40) |  // Move byte 6 to byte 1
                   ((value & 0xFF00000000000000ULL) >> 56);   // Move byte 7 to byte 0
        default:
            // Unsupported size
            return value;
    }
}

// Helper function for 64-bit integers using the general function
uint64_t reverse_endianness_64(uint64_t value) {
    return reverse_endianness(value, 64);
}

// Helper function for 32-bit integers using the general function
uint32_t reverse_endianness_32(uint32_t value) {
    return (uint32_t)reverse_endianness((uint64_t)value, 32);
}

// Helper function for 16-bit integers using the general function
uint16_t reverse_endianness_16(uint16_t value) {
    return (uint16_t)reverse_endianness((uint64_t)value, 16);
}

// String representation of mac address
char* mac_address_to_string(uint8_t *address) {
    char* string = (char*)malloc(18 * sizeof(char));
    sprintf(string, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
        address[0], address[1], address[2], address[3], address[4], address[5]);
    return string;
}

// Convert timeval to octets
void timeval_to_octets(struct timeval *tv, uint8_t *buffer_sec, uint8_t *buffer_nsec) {
    //ESP_LOGI(TAG, "timeval_to_octets: %lld.%ld", tv->tv_sec, tv->tv_usec);
    int64_t tv_sec = (int64_t)tv->tv_sec;
    int64_t tv_nsec = (int64_t)tv->tv_usec * 1000L;
    memcpy(buffer_sec, &tv_sec, 6);
    memcpy(buffer_nsec, &tv_nsec, 4);
    //ESP_LOG_BUFFER_HEX("sec:", buffer_sec, (6));
    //ESP_LOG_BUFFER_HEX("nsec:", buffer_nsec, (4));
    reverse_octets(buffer_sec, (6));
    reverse_octets(buffer_nsec, (4));
    //ESP_LOG_BUFFER_HEX("rev_sec:", buffer_sec, (6));
    //ESP_LOG_BUFFER_HEX("rev_nsec:", buffer_nsec, (4));
}