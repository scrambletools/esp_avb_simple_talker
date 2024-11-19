#include "utils.h"

// Define logging tag
static const char *TAG = "UTILS";

const char* get_frame_type_name(FrameType type) 
{
   switch (type) 
   {
      case FrameTypeAdpdu: return "ADPDU";
      case FrameTypeAecpdu: return "AECPDU";
	  case FrameTypeAcmpdu: return "ACMPDU";
   }
	 return "UNKNOWN";
}

void binary_printf(int v)
{
    unsigned int mask=1<<((sizeof(int)<<3)-1);
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
}

void print_frame(FrameType type, eth_frame_t *frame, ssize_t size) {

    static const int MINSIZE = 14; // 46
    static const char *TAG = "UTIL";
    if (size < MINSIZE) {
        ESP_LOGI(TAG, "Can't print frame, too small.");
    }
    else {
        ESP_LOGI(TAG, "*** Print Frame - %s (%d) ***", get_frame_type_name(type), size);
                ESP_LOG_BUFFER_HEX("           destination", &frame->header.dest, (6));
                ESP_LOG_BUFFER_HEX("                source", &frame->header.src, (6));
                ESP_LOG_BUFFER_HEX("             etherType", &frame->header.type, (2));
        switch (type) {
            case FrameTypeAdpdu:
                ESP_LOG_BUFFER_HEX("               subType", frame->payload, (1));
                ESP_LOG_BUFFER_HEX(" streamValidVerMsgType", frame->payload + 1, (1));
                ESP_LOG_BUFFER_HEX("  validTimeCtrlDataLen", frame->payload + 2, (2));
                ESP_LOG_BUFFER_HEX("              entityID", frame->payload + 4, (8));
                ESP_LOG_BUFFER_HEX("         entityModelID", frame->payload + 12, (8));
                ESP_LOG_BUFFER_HEX("    entityCapabilities", frame->payload + 20, (4));
                ESP_LOG_BUFFER_HEX("   talkerStreamSources", frame->payload + 24, (2));
                ESP_LOG_BUFFER_HEX("    talkerCapabilities", frame->payload + 26, (2));
                ESP_LOG_BUFFER_HEX("   listenerStreamSinks", frame->payload + 28, (2));
                ESP_LOG_BUFFER_HEX("  listenerCapabilities", frame->payload + 30, (2));
                ESP_LOG_BUFFER_HEX("controllerCapabilities", frame->payload + 32, (4));
                ESP_LOG_BUFFER_HEX("        availableIndex", frame->payload + 36, (4));
                ESP_LOG_BUFFER_HEX("     gptpGrandmasterID", frame->payload + 40, (8));
                ESP_LOG_BUFFER_HEX("      gptpDomainNumber", frame->payload + 48, (1));
                ESP_LOG_BUFFER_HEX("         reserved8bits", frame->payload + 49, (1));
                ESP_LOG_BUFFER_HEX("    currentConfigIndex", frame->payload + 50, (2));
                ESP_LOG_BUFFER_HEX("  identifyControlIndex", frame->payload + 52, (2));
                ESP_LOG_BUFFER_HEX("        interfaceIndex", frame->payload + 54, (2));
                ESP_LOG_BUFFER_HEX("         associationID", frame->payload + 56, (8));
                ESP_LOG_BUFFER_HEX("        reserved32bits", frame->payload + 64, (4));
                // Sonnettech has an additional empty 32bits at the end of the frame
                break;
            case FrameTypeAecpdu:
                ESP_LOG_BUFFER_HEX("PYLD", frame->payload + 14, (6));
                break;
            case FrameTypeAcmpdu:
                ESP_LOG_BUFFER_HEX("PYLD", frame->payload + 14, (6));
                break;
        }
        ESP_LOGI(TAG, "*** End of Frame ***");
    }
}