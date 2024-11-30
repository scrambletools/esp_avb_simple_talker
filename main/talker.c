#include "talker.h"

// Managed mac addresses
static uint8_t local_eth_mac_addr[ETH_ADDR_LEN] = { 0 };
static uint8_t local_wifi_mac_addr[ETH_ADDR_LEN] = { 0 };
static uint8_t controller_mac_addr[ETH_ADDR_LEN] = { 0 };
static uint8_t listener_mac_addr[ETH_ADDR_LEN] = { 0 }; // remote device
static uint8_t talker_mac_addr[ETH_ADDR_LEN] = { 0 }; // remote device

// L2tap file descriptors for each Ethertype; start out as invalid
static int l2tap_gptp_fd = INVALID_FD;
static int l2tap_avtp_fd = INVALID_FD;
static int l2tap_msrp_fd = INVALID_FD;
static int l2tap_mvrp_fd = INVALID_FD;

// Queue message structure
struct queue_message_t {
 uint8_t message_id;
 uint8_t message_type;
 uint8_t message_data[ 20 ];
};

// Queue for inter-task messaging
QueueHandle_t queue_for_main_task;
QueueHandle_t queue_for_gptp_task;
QueueHandle_t queue_for_avtp_task;
QueueHandle_t queue_for_msrp_task;
QueueHandle_t queue_for_mvrp_task;

// Ethernet driver handle
static esp_eth_handle_t eth_handle = NULL;

// Define logging tag
static const char *TAG = "TALKER";

// gPTP communication management task
static void gptp_task(void *pvParameters) {

    // Define task specific logging tag
    //char *TAG = (char*)malloc(16 * sizeof(char));
    //sprintf(TAG, "WATCH-%s", get_ethertype_name(ethertype));
    static const char *TAG = "TASK-GPTP";
    ESP_LOGI(TAG, "Started GPTP Management Task.");

    // Message for rx queue
    //struct queue_message_t message_for_gptp_task = {0,0,{0}};
    struct queue_message_t *p_message_for_gptp_task; //= &message_for_gptp_task;

    // Create a message to send to tx queue
    struct queue_message_t message_for_main_task = {0,0,{0}};
    struct queue_message_t *p_message_for_main_task = &message_for_main_task;

    // Create a queue capable of containing 10 pointers to queue_message_t structures.
    // These should be passed by pointer as they contain a lot of data.
    queue_for_main_task = xQueueCreate(10, sizeof(struct queue_message_t *));
    if (queue_for_main_task == 0) {
        ESP_LOGE(TAG, "Failed to create the queue: gptp->main.");
        goto error;
    }

    // Send a pointer to a struct queue_message_t object.  Don't block if the queue is full.
    xQueueSend(queue_for_main_task, (void *) &p_message_for_main_task, (TickType_t) 0);

    // Listen loop
    while (1) {

        // Check if queue_for_gptp_task has an incoming message
        if (queue_for_gptp_task != 0) {
            // Receive a message on the queue. Don't block if a message is not available.
            if (xQueueReceive(queue_for_gptp_task, &(p_message_for_gptp_task), (TickType_t) 0)) {
                // message_for_gptp_task now points to the struct queue_message_t variable posted
                // by another task, and the item is removed from the queue.
                // you can do something with the message here.
                ESP_LOGI(TAG, "I got a text!");
            }
        }

        // Get the frame from the fd, detect the frame type and set the timestamp of receipt
        eth_frame_t frame;
        if (get_frame(l2tap_gptp_fd, ethertype_gptp, &frame) == ESP_FAIL) {
            break;
        }

        // Log the frame
        print_frame(&frame, PRINT_SUMMARY);

        // Handle each type of frame that needs action
        switch(frame.frame_type) {
            case avb_frame_gptp_pdelay_request:
                send_gptp_pdelay_response(&frame);
                break;
            case avb_frame_gptp_pdelay_response:
                // check if the sequence_id matches the last request sent
                // if so, save the requeset_receipt_timestamp from the response
                // and set the gptp_last_pdelay_response_timestamp
                // if not, discard the response
                break;
            case avb_frame_gptp_pdelay_response_follow_up:
                // check if the sequence_id matches the last request sent
                // if so, save the response_origin_timestamp from the response
                // if not, discard the response follow up
                break;
            default:
                // nuthin
        }
    }
error:
    vTaskDelete(NULL);
}

// Send gPTP peer delay response and follow up messages
void send_gptp_pdelay_response(eth_frame_t * req_frame) {

    // Create a new frame, set the header and append the response message
    eth_frame_t frame;
    set_frame_header(lldp_mcast_mac_addr, ethertype_gptp, &frame);
    frame.frame_type = avb_frame_gptp_pdelay_response;
    append_gptpdu(frame.frame_type, &frame);

    // -- Process response based on request frame data --

    // Insert the sequence_id from the request frame
    memcpy(frame.payload + 30, req_frame->payload + 30, (2));

    // Insert the requeset_receipt_timestamp from the request time of receipt
    uint8_t timestamp_sec[6] = {};
    uint8_t timestamp_nsec[4] = {};
    timeval_to_octets(&req_frame->time_received, timestamp_sec, timestamp_nsec);
    memcpy(frame.payload + 34, &timestamp_sec, (6)); // requestReceiptTimestamp(sec)
    memcpy(frame.payload + 40, &timestamp_nsec, (4)); // requestReceiptTimestamp(nanosec)

    // Insert the requesting_source_port_identity from the request frame clock_identity
    memcpy(frame.payload + 44, req_frame->payload + 20, (8));
    // Insert the requesting_source_port_id from the request frame source_port_id
    memcpy(frame.payload + 52, req_frame->payload + 28, (2));

    // Send response
    send_frame(&frame);
    print_frame(&frame, PRINT_VERBOSE);

    // --- Process response follow up based on request and response frame data ---
    
    // Reuse the frame; change payload to response follow up
    // Re-insert the sequence_id, requesting_source_port_identity, and requesting_source_port_id
    frame.frame_type = avb_frame_gptp_pdelay_response_follow_up;
    append_gptpdu(frame.frame_type, &frame);
    memcpy(frame.payload + 30, req_frame->payload + 30, (2));
    memcpy(frame.payload + 44, req_frame->payload + 20, (8));
    memcpy(frame.payload + 52, req_frame->payload + 28, (2));

    // Insert the response_origin_timestamp from the response time of transmission
    // gptp_timestamp_last_sent_pdelay_response was updated when the response was sent
    timeval_to_octets(&gptp_timestamp_last_sent_pdelay_response, timestamp_sec, timestamp_nsec);
    memcpy(frame.payload + 34, &timestamp_sec, (6)); // responseOriginTimestamp(sec)
    memcpy(frame.payload + 40, &timestamp_nsec, (4)); // responseOriginTimestamp(nanosec)

    // how to extract int64 from timestamp in buffer
    int64_t backto64 = octets_to_uint64(timestamp_sec, sizeof(timestamp_sec));
    backto64 += octets_to_uint64(timestamp_nsec, sizeof(timestamp_nsec));
    
    // Send response follow up
    send_frame(&frame);
    print_frame(&frame, PRINT_VERBOSE);
}

// AVTP communication management task
static void avtp_task(void *pvParameters) {

    // Define task specific logging tag
    static const char *TAG = "TASK-AVTP";
    ESP_LOGI(TAG, "Started AVTP Management Task.");

    // Message for rx queue
    struct queue_message_t *p_message_for_avtp_task;

    // Create a message to send to tx queue
    struct queue_message_t message_for_main_task = {0, 0, { 0 }};
    struct queue_message_t *p_message_for_main_task = &message_for_main_task;

    // Create a queue capable of containing 10 pointers to queue_message_t structures.
    // These should be passed by pointer as they contain a lot of data.
    queue_for_main_task = xQueueCreate(10, sizeof(struct queue_message_t *));
    if (queue_for_main_task == 0) {
        ESP_LOGE(TAG, "Failed to create the queue: avtp->main.");
        goto error;
    }

    // Send a pointer to a struct queue_message_t object.  Don't block if the queue is full.
    xQueueSend(queue_for_main_task, (void *) &p_message_for_main_task, (TickType_t) 0);

    // Listen loop
    while (1) {

        // Check if queue_for_avtp_task has an incoming message
        if (queue_for_avtp_task != 0) {
            // Receive a message on the queue. Don't block if a message is not available.
            if (xQueueReceive(queue_for_avtp_task, &(p_message_for_avtp_task), (TickType_t) 0)) {
                // message_for_avtp_task now points to the struct queue_message_t variable posted
                // by another task, and the item is removed from the queue.
                // you can do something with the message here.
                ESP_LOGI(TAG, "I got a text!");
            }
        }
        // Get the frame from the fd, detect the frame type and set the timestamp of receipt
        eth_frame_t frame;
        if (get_frame(l2tap_avtp_fd, ethertype_avtp, &frame) == ESP_FAIL) {
            break;
        }
        // Log the received frame
        print_frame(&frame, PRINT_SUMMARY);

        // Do something with the frame
    }
error:
    vTaskDelete(NULL);
}

// MSRP communication management task
static void msrp_task(void *pvParameters) {

    // Define task specific logging tag
    static const char *TAG = "TASK-MSRP";
    ESP_LOGI(TAG, "Started MSRP Management Task.");

    // Message for rx queue
    struct queue_message_t *p_message_for_msrp_task;

    // Create a message to send to tx queue
    struct queue_message_t message_for_main_task = { 0, 0, { 0 }};
    struct queue_message_t *p_message_for_main_task = &message_for_main_task;

    // Create a queue capable of containing 10 pointers to queue_message_t structures.
    // These should be passed by pointer as they contain a lot of data.
    queue_for_main_task = xQueueCreate(10, sizeof(struct queue_message_t *));
    if (queue_for_main_task == 0) {
        ESP_LOGE(TAG, "Failed to create the queue: msrp->main.");
        goto error;
    }

    // Send a pointer to a struct queue_message_t object.  Don't block if the queue is full.
    xQueueSend(queue_for_main_task, (void *) &p_message_for_main_task, (TickType_t) 0);

    // Listen loop
    while (1) {

        // Check if queue_for_msrp_task has an incoming message
        if (queue_for_msrp_task != 0) {
            // Receive a message on the queue. Don't block if a message is not available.
            if (xQueueReceive(queue_for_msrp_task, &(p_message_for_msrp_task), (TickType_t) 0)) {
                // message_for_msrp_task now points to the struct queue_message_t variable posted
                // by another task, and the item is removed from the queue.
                // you can do something with the message here.
                ESP_LOGI(TAG, "I got a text!");
            }
        }
        // Get the frame from the fd, detect the frame type and set the timestamp of receipt
        eth_frame_t frame;
        if (get_frame(l2tap_msrp_fd, ethertype_msrp, &frame) == ESP_FAIL) {
            break;
        }
        // Log the received frame
        print_frame(&frame, PRINT_SUMMARY);

        // Do something with the frame
    }
error:
    vTaskDelete(NULL);
}

// MVRP communication management task
static void mvrp_task(void *pvParameters) {

    // Define task specific logging tag
    static const char *TAG = "TASK-MVRP";
    ESP_LOGI(TAG, "Started MVRP Management Task.");

    // Message for rx queue
    struct queue_message_t *p_message_for_mvrp_task;

    // Create a message to send to tx queue
    struct queue_message_t message_for_main_task = {0,0,{0}};
    struct queue_message_t *p_message_for_main_task = &message_for_main_task;

    // Create a queue capable of containing 10 pointers to queue_message_t structures.
    // These should be passed by pointer as they contain a lot of data.
    queue_for_main_task = xQueueCreate(10, sizeof(struct queue_message_t *));
    if (queue_for_main_task == 0) {
        ESP_LOGE(TAG, "Failed to create the queue: mvrp->main.");
        goto error;
    }

    // Send a pointer to a struct queue_message_t object.  Don't block if the queue is full.
    xQueueSend(queue_for_main_task, (void *) &p_message_for_main_task, (TickType_t) 0);

    // Listen loop
    while (1) {

        // Check if queue_for_mvrp_task has an incoming message
        if (queue_for_mvrp_task != 0) {
            // Receive a message on the queue. Don't block if a message is not available.
            if (xQueueReceive(queue_for_mvrp_task, &(p_message_for_mvrp_task), (TickType_t) 0)) {
                // message_for_mvrp_task now points to the struct queue_message_t variable posted
                // by another task, and the item is removed from the queue.
                // you can do something with the message here.
                ESP_LOGI(TAG, "I got a text!");
            }
        }
        // Get the frame from the fd, detect the frame type and set the timestamp of receipt
        eth_frame_t frame;
        if (get_frame(l2tap_mvrp_fd, ethertype_mvrp, &frame) == ESP_FAIL) {
            break;
        }
        // Log the received frame
        print_frame(&frame, PRINT_SUMMARY);

        // Do something with the frame
    }
error:
    vTaskDelete(NULL);
}

// Opens and configures L2 TAP file descriptor; flags=0 is blocking, 1 is non-blocking
int init_l2tap_fd(int flags, ethertype_t ethertype)
{ 
    // Open a file descriptor using the flags
    int fd = open("/dev/net/tap", flags);
    if (fd < 0) {
        ESP_LOGE(TAG, "Unable to open L2 TAP interface, errno %d", errno);
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

// Setup l2tap file descripters (fd) for all necessary Ethertypes
esp_err_t init_all_l2tap_fds() {
    //Setup gPTP fd
    if (l2tap_gptp_fd == INVALID_FD) {
        if ((l2tap_gptp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_gptp)) == INVALID_FD) {
            return ESP_FAIL;
        }
    }
    // Setup AVTP fd
    if (l2tap_avtp_fd == INVALID_FD) {
        if ((l2tap_avtp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_avtp)) == INVALID_FD) {
            return ESP_FAIL;
        }
    }
    // Setup MSRP fd
    if (l2tap_msrp_fd == INVALID_FD) {
        if ((l2tap_msrp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_msrp)) == INVALID_FD) {
            return ESP_FAIL;
        }
    }
    // Setup MVRP fd
    if (l2tap_mvrp_fd == INVALID_FD) {
        if ((l2tap_mvrp_fd = init_l2tap_fd(O_NONBLOCK, ethertype_mvrp)) == INVALID_FD) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

// Close l2tap file descripters (fd) for all necessary Ethertypes
void close_all_l2tap_fds() {
    // Close gPTP fd
    if (l2tap_gptp_fd != INVALID_FD) {
        close(l2tap_gptp_fd);
    }
    // Close AVTP fd
    if (l2tap_avtp_fd != INVALID_FD) {
        close(l2tap_avtp_fd);
    }
    // Close MSRP fd
    if (l2tap_msrp_fd != INVALID_FD) {
        close(l2tap_msrp_fd);
    }
    // Close MVRP fd
    if (l2tap_mvrp_fd != INVALID_FD) {
        close(l2tap_mvrp_fd);
    }
}

// Get the frame from the fd
esp_err_t get_frame(int fd, ethertype_t ethertype, eth_frame_t *frame) {
    
    // Create timeval
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    // Create an fd set
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // Create a buffer to read the frame from the fd
    uint8_t rx_buffer[1500]; //128

    // Use select to setup a timeout for the read
    int result = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (result > 0) {

        // Read a frame from the fd, if any
        ssize_t len = read(fd, rx_buffer, sizeof(rx_buffer));
        if (len > 0) {
            // Copy frame data to frame struct
            memcpy(frame, (eth_frame_t *)rx_buffer, len);

            // Remember the timestamp of receipt
            gettimeofday(&frame->time_received, NULL);

            // Detect the frame type for proper handling
            avb_frame_type_t frame_type = avb_frame_unknown;
            switch (ethertype) {
                case ethertype_gptp:
                    frame_type = detect_gptp_frame_type(frame, len);
                    break;
                case ethertype_avtp:
                    uint8_t subtype;
                    memcpy(&subtype, &frame->payload, (1));
                    if (subtype == avtp_subtype_adp || subtype == avtp_subtype_aecp || subtype == avtp_subtype_acmp) {
                        frame_type = detect_atdecc_frame_type(frame, len);
                    }
                    else {
                        frame_type = detect_avtp_frame_type(&ethertype, frame, len);
                    }
                    break;
                case ethertype_msrp:
                    frame_type = detect_avtp_frame_type(&ethertype, frame, len);
                    break;
                case ethertype_mvrp:
                    frame_type = detect_avtp_frame_type(&ethertype, frame, len);
                    break;
                default:
                    ESP_LOGE(TAG, "Cannot detect frame with an unkown Ethertype, 0x%x.", ethertype);
                    return ESP_FAIL;
            }
            
            // Fill in the frame type and payload size
            frame->frame_type = frame_type;
            frame->payload_size = len - ETH_HEADER_LEN;
        } else {
            ESP_LOGE(TAG, "L2 TAP fd %d read error: errno %d", fd, errno);
            return ESP_FAIL;
        }
    } else if (result == 0) {
        ESP_LOGD(TAG, "L2 TAP select timeout");
    } else {
        ESP_LOGE(TAG, "L2 TAP select error: errno %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Set the destination, source (local mac) and Ethertype in a frame, based on params
void set_frame_header(uint8_t *dest_addr, ethertype_t ethertype, eth_frame_t *frame) {
    // Add the destination
    memcpy(frame->header.dest.addr, dest_addr, ETH_ADDR_LEN);
    // Add local mac address as the source
    memcpy(frame->header.src.addr, local_eth_mac_addr, ETH_ADDR_LEN);
    // Add the Ethertype
    ethertype_t type = ntohs(ethertype);
    memcpy(&frame->header.type, &type, sizeof(ethertype));
}

// Send an Ethernet frame
esp_err_t send_frame(eth_frame_t *frame) {
    
    // Select the fd based on the frame's Ethertype
    ethertype_t ethertype = ntohs(frame->header.type);

    // Select the fd based on the given Ethertype
    int fd;
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
            ESP_LOGE(TAG, "Cannot send frame with an unkown Ethertype, 0x%x.", ethertype);
            return ESP_FAIL;
    }

    // Save timestamp for follow up
    if (frame->frame_type == avb_frame_gptp_pdelay_response) {
        gettimeofday(&gptp_timestamp_last_sent_pdelay_response, NULL);
    }
    else {
        gettimeofday(&gptp_timestamp_last_sent_sync, NULL);
    }
    // Send away!
    ssize_t len = write(fd, frame, frame->payload_size + ETH_HEADER_LEN);
    if (len < 0) {
        ESP_LOGE(TAG, "L2 TAP fd %d write error, errno, %d", fd, errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Print out the frame (format: 0 for short (default), 1 for long format)
void print_frame_of_type(avb_frame_type_t type, eth_frame_t *frame, int format) {
    if (frame->payload_size < 2) {
        ESP_LOGI(TAG, "Can't print frame, payload is too small: %d", frame->payload_size);
    }
    else {
        switch (type) {
            case avb_frame_gptp_announce ... avb_frame_gptp_pdelay_response_follow_up:
                print_gptp_frame(type, frame, format);
                break;
            case avb_frame_adp_entity_available ... avb_frame_acmp_get_rx_state_response:
                print_atdecc_frame(type, frame, format);
                break;
            case avb_frame_avtp_stream ... avb_frame_mvrp_vlan_identifier:
                print_avtp_frame(type, frame, format);
                break;
            default:
                ESP_LOGI(TAG, "Can't print frame of unknown frame type: 0x%x", type);
        }
    }
}

// Print out the frame; get type from frame
void print_frame(eth_frame_t *frame, int format) {
    print_frame_of_type(frame->frame_type, frame, format);
}

// Starup the talker
void start_talker(esp_netif_iodriver_handle handle) {
    
    ESP_LOGI(TAG, "Starting Talker...");
    
    // Set the Ethernet driver handle (passed from main app)
    eth_handle = handle;

    // Set the local Ethernet mac address
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, local_eth_mac_addr);

    // Open and configure L2 TAP file descriptors
    esp_err_t err = init_all_l2tap_fds();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize FDs, %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "All FDs are open.");
        
        // Start watching for gPTP frames
        xTaskCreate(gptp_task, "gptp_task", 6144, NULL, 5, NULL);

        // Start watching for AVTP frames
        //xTaskCreate(avtp_task, "avtp_task", 6144, NULL, 5, NULL);

        // Start watching for MSRP frames
        //xTaskCreate(msrp_task, "msrp_task", 6144, NULL, 5, NULL);

        // Start watching for MVRP frames
        //xTaskCreate(mvrp_task, "mvrp_task", 6144, NULL, 5, NULL);
    }
}

// Stop the talker
void stop_talker() {
    ESP_LOGI(TAG, "Stopping Talker...");
    close_all_l2tap_fds();
    ESP_LOGI(TAG, "All FDs are closed.");
}