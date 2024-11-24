#include "talker.h"

// L2tap file descriptors for each Ethertype
static int l2tap_gptp_fd;
static int l2tap_avtp_fd;
static int l2tap_msrp_fd;
static int l2tap_mvrp_fd;

// Commonly used mac addresses
static const uint8_t bcast_mac_addr[] = { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 };
//static const uint8_t spantree_mac_addr[] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 };
//static const uint8_t lldp_mcast_mac_addr[] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };

// Ethernet driver handle
static esp_eth_handle_t eth_handle = NULL;

// Define logging tag
static const char *TAG = "TALKER";

// Opens and configures L2 TAP file descriptor; flags=0 is blocking, 1 is non-blocking
int init_l2tap_fd(int flags, ethertype_t ethertype)
{
    // Open a file descriptor using the flags
    int fd = open("/dev/net/tap", flags);
    if (fd < 0) {
        ESP_LOGE(TAG, "Unable to open L2 TAP interface: errno %d", errno);
        goto error;
    }
    ESP_LOGI(TAG, "/dev/net/tap fd %d successfully opened", fd);

    // Configure the fd to use the default Ethernet interface
    int result;
    if ((result = ioctl(fd, L2TAP_S_INTF_DEVICE, ETH_INTERFACE)) == INVALID_FD) {
        ESP_LOGE(TAG, "Unable to bound L2 TAP fd %d with Ethernet device: errno %d", fd, errno);
        goto error;
    }
    ESP_LOGI(TAG, "L2 TAP fd %d successfully bound to the default Ethernet interface", fd);

    // Configure Ethernet frames we want to filter out
    if ((result = ioctl(fd, L2TAP_S_RCV_FILTER, &ethertype)) == INVALID_FD) {
        ESP_LOGE(TAG, "Unable to configure fd %d Ethernet type receive filter: errno %d", fd, errno);
        goto error;
    }
    ESP_LOGI(TAG, "L2 TAP fd %d Ethernet type filter configured to 0x%x", fd, ethertype);

    return fd;
error:
    if (fd != INVALID_FD) {
        close(fd);
    }
    return INVALID_FD;
}

// frame logger
static void frame_watcher_task(void *pvParameters)
{
    // Grab config from the task params
    ethertype_t *type = (ethertype_t*)pvParameters;
    ethertype_t ethertype = *type;

    ESP_LOGI(TAG, "Started task to log frames with ethertype %d.", ethertype);

    uint8_t rx_buffer[128];
    int fd;

    // Select the fd based on the given Ethertype
    switch (ethertype) {
        case ethertype_gptp:
            fd = l2tap_gptp_fd;
            break;
        case ethertype_msrp:
            fd = l2tap_msrp_fd;
            break;
        case ethertype_mvrp:
            fd = l2tap_mvrp_fd;
            break;
        case ethertype_avtp:
            fd = l2tap_avtp_fd;
            break;
        default:
            ESP_LOGE(TAG, "Cannot log an unkown Ethertype: %d.", ethertype);
            goto error;
    }

    // Listen loop
    while (1) {
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        // Create and fd set
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        int result = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (result > 0) {
            ssize_t len = read(fd, rx_buffer, sizeof(rx_buffer));
            if (len > 0) {
                eth_frame_t *recv_msg = (eth_frame_t *)rx_buffer;
                ESP_LOGI("T-GPTP", "fd %d received %d bytes from %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", fd,
                            len, recv_msg->header.src.addr[0], recv_msg->header.src.addr[1], recv_msg->header.src.addr[2],
                            recv_msg->header.src.addr[3], recv_msg->header.src.addr[4], recv_msg->header.src.addr[5]);
                // Set the frame type for proper handling
                avb_frame_type_t frame_type = avb_frame_unknown;
                switch (ethertype) {
                    case ethertype_gptp:
                        frame_type = detect_gptp_frame_type(recv_msg, len);
                        break;
                    case ethertype_avtp:
                        uint8_t subtype;
                        memcpy(&subtype, &recv_msg->payload, (1));
                        if (subtype == avtp_subtype_adp || subtype == avtp_subtype_aecp || subtype == avtp_subtype_acmp) {
                            frame_type = detect_atdecc_frame_type(recv_msg, len);
                        }
                        else {
                            frame_type = detect_avtp_frame_type(&ethertype, recv_msg, len);
                        }
                        break;
                    case ethertype_msrp:
                        frame_type = detect_avtp_frame_type(&ethertype, recv_msg, len);
                        break;
                    case ethertype_mvrp:
                        frame_type = detect_avtp_frame_type(&ethertype, recv_msg, len);
                        break;
                    default:
                        ESP_LOGE(TAG, "Cannot detect frame with an unkown Ethertype: %d.", ethertype);
                        goto error;
                }
                print_frame(frame_type, recv_msg, len);
            } else {
                ESP_LOGE(TAG, "L2 TAP fd %d read error: errno %d", fd, errno);
                break;
            }
        } else if (result == 0) {
            ESP_LOGD(TAG, "L2 TAP select timeout");
        } else {
            ESP_LOGE(TAG, "L2 TAP select error: errno %d", errno);
            break;
        }
    }
    close(fd);
error:
    vTaskDelete(NULL);
}

// Setup l2tap file descripters for all Ethertypes
esp_err_t init_all_l2tap_fds() {
    //Setup gPTP fd
    if ((l2tap_gptp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_gptp)) == INVALID_FD) {
        return ESP_FAIL;
    }
    // Setup AVTP fd
    if ((l2tap_avtp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_avtp)) == INVALID_FD) {
        return ESP_FAIL;
    }
    // Setup MSRP fd
    if ((l2tap_msrp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_msrp)) == INVALID_FD) {
        return ESP_FAIL;
    }
    // Setup MVRP fd
    if ((l2tap_mvrp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_mvrp)) == INVALID_FD) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void print_frame(avb_frame_type_t type, eth_frame_t *frame, ssize_t size) {

    if (size < ETH_HEADER_LEN) {
        ESP_LOGI(TAG, "Can't print frame, too small.");
    }
    else {
        switch (type) {
            case avb_frame_gptp_announce ... avb_frame_gptp_pdelay_follow_up:
                print_gptp_frame(type, frame, size);
                break;
            case avb_frame_adp ... avb_frame_acmp:
                print_atdecc_frame(type, frame, size);
                break;
            case avb_frame_avtp_stream ... avb_frame_mvrp_vlan_identifier:
                print_avtp_frame(type, frame, size);
                break;
            default:
                ESP_LOGI(TAG, "Can't print frame of unknown frame type.");
        }
    }
}

// Send a frame
esp_err_t send_frame(eth_frame_t *frame) {
    int fd;
    // Select the fd based on the frame's Ethertype
    ethertype_t ethertype = ntohs(frame->header.type);

    // Select the fd based on the given Ethertype
    switch (ethertype) {
        case ethertype_gptp:
            fd = l2tap_gptp_fd;
            break;
        case ethertype_msrp:
            fd = l2tap_msrp_fd;
            break;
        case ethertype_mvrp:
            fd = l2tap_mvrp_fd;
            break;
        case ethertype_avtp:
            fd = l2tap_avtp_fd;
            break;
        default:
            ESP_LOGE(TAG, "Cannot send frame with an unkown Ethertype: %d.", ethertype);
            return ESP_FAIL;
    }

    // Send away!
    print_frame(avb_frame_adp, frame, frame->payload_size + ETH_HEADER_LEN);
    ssize_t len = write(fd, frame, frame->payload_size + ETH_HEADER_LEN);
    if (len < 0) {
        ESP_LOGE(TAG, "L2 TAP fd %d write error: errno: %d", fd, errno);
        return ESP_FAIL;
    }
    //close(fd);
    return ESP_OK;
}

// Send Entity Available message using the Adpdu class
void send_entity_available()
{
    eth_frame_t frame;
    uint8_t mac_addr[ETH_ADDR_LEN];
    uint16_t ethertype = ntohs(ethertype_avtp);
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    memcpy(frame.header.src.addr, mac_addr, ETH_ADDR_LEN);
    memcpy(frame.header.dest.addr, bcast_mac_addr, ETH_ADDR_LEN);
    memcpy(&frame.header.type, &ethertype, sizeof(ethertype));
    append_adpdu(entity_available, &frame);
    send_frame(&frame);
}

// Starup the talker
void start_talker(esp_netif_iodriver_handle handle) {
    
    ESP_LOGI(TAG, "Starting Talker...");
    
    // Set the Ethernet driver handle
    eth_handle = handle;

    // Initialize L2 TAP VFS interface
    ESP_ERROR_CHECK(esp_vfs_l2tap_intf_register(NULL));

    // Open and configure L2 TAP file descriptors
    esp_err_t err = init_all_l2tap_fds();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize FDs: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "All FDs are open.");
        
        // Start watching for gPTP frames
        static ethertype_t type1 = ethertype_gptp;
        xTaskCreate(frame_watcher_task, "watch_gptp", 6144, &type1, 5, NULL);
        static ethertype_t type2 = ethertype_avtp;
        xTaskCreate(frame_watcher_task, "watch_avtp", 6144, &type2, 5, NULL);
        static ethertype_t type3 = ethertype_msrp;
        xTaskCreate(frame_watcher_task, "watch_msrp", 6144, &type3, 5, NULL);
        static ethertype_t type4 = ethertype_mvrp;
        xTaskCreate(frame_watcher_task, "watch_mvrp", 6144, &type4, 5, NULL);

        // Send out stuff
        // while (true) {
        //     // Send Entity Available message
        //     send_entity_available();
        //     ESP_LOGI(TAG, "Entity Available message sent.");
        //     vTaskDelay(pdMS_TO_TICKS(10000)); // repeat every ~10sec
        // }
    }
}