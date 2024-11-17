#include "talker.h"

// for l2tap
#define ETH_INTERFACE   "ETH_DEF"
#define INVALID_FD      -1
#define AVTP_ETHER_TYPE 0x22f0

// Define logging tag
static const char *TAG = "TALKER";

static const uint8_t bcast_mac_addr[] = { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 };

int l2tap_fd;

// Opens and configures L2 TAP file descriptor, flags=0 is blocking, 1 is non-blocking
static int init_l2tap_fd(int flags, uint16_t eth_type)
{
    int fd = open("/dev/net/tap", flags);
    if (fd < 0) {
        ESP_LOGE(TAG, "Unable to open L2 TAP interface: errno %d", errno);
        goto error;
    }
    ESP_LOGI(TAG, "/dev/net/tap fd %d successfully opened", fd);

    // Configure Ethernet interface on which to get raw frames
    int ret;
    if ((ret = ioctl(fd, L2TAP_S_INTF_DEVICE, ETH_INTERFACE)) == -1) {
        ESP_LOGE(TAG, "Unable to bound L2 TAP fd %d with Ethernet device: errno %d", fd, errno);
        goto error;
    }
    ESP_LOGI(TAG, "L2 TAP fd %d successfully bound to `%s`", fd, ETH_INTERFACE);

    // Configure Ethernet frames we want to filter out
    if ((ret = ioctl(fd, L2TAP_S_RCV_FILTER, &eth_type)) == -1) {
        ESP_LOGE(TAG, "Unable to configure fd %d Ethernet type receive filter: errno %d", fd, errno);
        goto error;
    }
    ESP_LOGI(TAG, "L2 TAP fd %d Ethernet type filter configured to 0x%x", fd, eth_type);

    return fd;
error:
    if (fd != INVALID_FD) {
        close(fd);
    }
    return INVALID_FD;
}

// frame logger
static void eth_frame_logger(void *pvParameters)
{
    ESP_LOGI(TAG, "Started task to log frames.");
    uint8_t rx_buffer[128];
    int eth_tap_fd;
    
    // Open and configure L2 TAP File descriptor
    if ((eth_tap_fd = init_l2tap_fd(1, AVTP_ETHER_TYPE)) == INVALID_FD) {
        goto error;
    }

    while (1) {
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(eth_tap_fd, &rfds);

        int ret_sel = select(eth_tap_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret_sel > 0) {
            ssize_t len = read(eth_tap_fd, rx_buffer, sizeof(rx_buffer));
            if (len > 0) {
                eth_frame_t *recv_msg = (eth_frame_t *)rx_buffer;
                ESP_LOGI(TAG, "fd %d received %d bytes from %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", eth_tap_fd,
                            len, recv_msg->header.src.addr[0], recv_msg->header.src.addr[1], recv_msg->header.src.addr[2],
                            recv_msg->header.src.addr[3], recv_msg->header.src.addr[4], recv_msg->header.src.addr[5]);
            } else {
                ESP_LOGE(TAG, "L2 TAP fd %d read error: errno %d", eth_tap_fd, errno);
                break;
            }
        } else if (ret_sel == 0) {
            ESP_LOGD(TAG, "L2 TAP select timeout");
        } else {
            ESP_LOGE(TAG, "L2 TAP select error: errno %d", errno);
            break;
        }
    }
    close(eth_tap_fd);
error:
    vTaskDelete(NULL);
}

// Initialize the Talker/Listener
esp_err_t initialize(esp_eth_handle_t handle)
{
    // Initialize L2 TAP VFS interface
    ESP_ERROR_CHECK(esp_vfs_l2tap_intf_register(NULL));

    if ((l2tap_fd = init_l2tap_fd(1, AVTP_ETHER_TYPE)) == INVALID_FD) {
        return ESP_FAIL;
    }
    eth_handle = handle;
    
    return ESP_OK;
}

// Send a frame
esp_err_t send_frame(eth_frame_t frame)
{
    print_frame(FrameTypeAdpdu, frame, frame.payload_size + ETH_HEADER_LEN);
    ssize_t ret = write(l2tap_fd, &frame, frame.payload_size + ETH_HEADER_LEN);
    if (ret < 0) {
        ESP_LOGE(TAG, "L2 TAP fd %d write error: errno: %d", l2tap_fd, errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Send Entity Available message using the Adpdu class
void send_entity_available()
{
    eth_frame_t frame;
    uint8_t mac_addr[ETH_ADDR_LEN];
    uint16_t ether_type = ntohs(AVTP_ETHER_TYPE);
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    memcpy(frame.header.src.addr, mac_addr, ETH_ADDR_LEN);
    memcpy(frame.header.dest.addr, bcast_mac_addr, ETH_ADDR_LEN);
    memcpy(&frame.header.type, &ether_type, sizeof(ether_type));
    append_adpdu(entity_available, &frame);
    send_frame(frame);
}