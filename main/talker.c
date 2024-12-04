#include "talker.h"

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
    static const char *TAG = "TASK-GPTP";
    ESP_LOGI(TAG, "Started GPTP Management Task.");

    // Get gPTP state and current GM info
    gptp_state_info_t *gptp_state = get_gptp_state();
    gptp_gm_info_t *current_gm = get_current_gm();
    gptp_pdelay_info_t *current_pdelay = get_current_pdelay();
    gptp_offset_info_t *current_offset = get_current_offset();

    // Message for rx queue
    struct queue_message_t *p_message_for_gptp_task;

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

    // Create a timer for pdelay requests
    const esp_timer_create_args_t pdelay_request_timer_args = {
            .callback = &send_gptp_pdelay_request,
            .name = "pdelay-request-timer" // for debugging
    };
    esp_timer_handle_t pdelay_request_timer;
    ESP_ERROR_CHECK(esp_timer_create(&pdelay_request_timer_args, &pdelay_request_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pdelay_request_timer, CONFIG_GM_PDELAY_REQUEST_PERIOD * 1000));

    // If GM capable then setup timers for local gm checks, 
    // as well as sending announce, sync and follow up messages
    if (CONFIG_GM_CAPABLE) {

        // Create a timer to check if local gm is needed
        const esp_timer_create_args_t local_gm_timer_args = {
                .callback = &check_local_gm,
                .name = "local-gm-timer" // for debugging
        };
        esp_timer_handle_t local_gm_timer;
        int local_gm_check_period = CONFIG_GM_TIMEOUT * current_gm->announce_period;
        ESP_ERROR_CHECK(esp_timer_create(&local_gm_timer_args, &local_gm_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(local_gm_timer, local_gm_check_period * 1000));

        // Create a timer for announce messages
        const esp_timer_create_args_t announce_timer_args = {
                .callback = &send_gptp_announce,
                .name = "announce-timer" // for debugging
        };
        esp_timer_handle_t announce_timer;
        ESP_ERROR_CHECK(esp_timer_create(&announce_timer_args, &announce_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(announce_timer, CONFIG_GM_ANNOUNCE_PERIOD * 1000));

        // Create a timer for sync messages
        const esp_timer_create_args_t sync_timer_args = {
                .callback = &send_gptp_sync,
                .name = "sync-timer" // for debugging
        };
        esp_timer_handle_t sync_timer;
        ESP_ERROR_CHECK(esp_timer_create(&sync_timer_args, &sync_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(sync_timer, CONFIG_GM_SYNC_PERIOD * 1000));
    }

    // Listen loop
    while (1) {

        // Check if queue_for_gptp_task has an incoming message
        if (queue_for_gptp_task != 0) {
            // Receive a message on the queue. Don't block if a message is not available.
            if (xQueueReceive(queue_for_gptp_task, &(p_message_for_gptp_task), (TickType_t) 0)) {
                // p_message_for_gptp_task now points to the struct queue_message_t variable posted
                // by another task, and the item is removed from the queue.
                // you can do something with the message here.
                ESP_LOGI(TAG, "Received queue message: ID=%d (Type=%d)", p_message_for_gptp_task->message_id, p_message_for_gptp_task->message_type);
            }
        }

        // Get next frame from the fd, detect the frame type and set the timestamp of receipt
        eth_frame_t frame;
        if (get_frame(ethertype_gptp, &frame) == ESP_FAIL) {
            break;
        }

        // Log all received frames
        //print_frame(&frame, PRINT_SUMMARY);

        // Handle each type of frame that needs action
        switch(frame.frame_type) {
            case avb_frame_gptp_pdelay_request:
                print_frame(&frame, PRINT_SUMMARY);
                send_gptp_pdelay_response(&frame);
                break;
            case avb_frame_gptp_pdelay_response:
                //print_frame(&frame, PRINT_SUMMARY);
                // Check if the sequence_id >= last request sent from local
                if (octets_to_uint(frame.payload + 30, 2) >= gptp_state->pdelay_request_sequence_id) {
                    // Save the requeset receipt timestamp from the response frame
                    current_pdelay->timestamp_request_receipt.tv_sec = octets_to_uint(frame.payload + 34, 6);
                    current_pdelay->timestamp_request_receipt.tv_usec = (int)(octets_to_uint(frame.payload + 40, 4) / 1000);
                    // Save the pdelay response receipt timestamp from the response frame
                    current_pdelay->timestamp_response_receipt.tv_sec = frame.time_received.tv_sec;
                    current_pdelay->timestamp_response_receipt.tv_usec = frame.time_received.tv_usec;
                }
                // If not, then discard the response
                break;
            case avb_frame_gptp_pdelay_response_follow_up:
                //print_frame(&frame, PRINT_SUMMARY);
                // Check if the sequence_id >= last request sent from local
                if (octets_to_uint(frame.payload + 30, 2) >= gptp_state->pdelay_request_sequence_id) {
                    // Save the response origin timestamp from the response frame
                    current_pdelay->timestamp_response_transmission .tv_sec = octets_to_uint(frame.payload + 34, 6);
                    current_pdelay->timestamp_response_transmission.tv_usec = (int)(octets_to_uint(frame.payload + 40, 4) / 1000);
                    // Calculate the peer delay and save it
                    uint64_t peer_delay = calculate_pdelay(current_pdelay);
                    current_pdelay->calculated_pdelay = peer_delay;
                }
                // If not, then discard the response follow up
                break;
            case avb_frame_gptp_announce:
                //print_frame(&frame, PRINT_SUMMARY);
                // Check if the announce message is for same GM as current GM, if so do nothing
                // If different GM, then evaluate BMCA and update current GM info if needed
                break;
            case avb_frame_gptp_sync:
                //print_frame(&frame, PRINT_SUMMARY);
                // Check if the sync message is for same GM as current GM, if so then update sync_sequence_id
                // If different GM, then discard the sync message
                break;
            case avb_frame_gptp_follow_up:
                //print_frame(&frame, PRINT_SUMMARY);
                // Check if the follow up message is for same GM as current GM
                // if so then update current_offset and adjust internal clock
                // If different GM, then discard the follow up message
                break;
            default:
        }
    }
error:
    vTaskDelete(NULL);
}

// Check if local GM needed
static void check_local_gm() {

    // Get needed gPTP variables
    gptp_state_info_t *gptp_state = get_gptp_state();
    gptp_gm_info_t *gm_list = get_gm_list();
    gptp_gm_info_t *current_gm = get_current_gm();

    // If all good do nothing  
    if (gptp_state->gm_is_present && gptp_state->gm_is_local) {
        return;
    }
    // If local GM but not present (unlikely), then set to present
    if (gptp_state->gm_is_local) {      
        gptp_state->gm_is_present = true;
        return;
    }
    // If GM is present but not local (most common case), then check if GM timed out 
    if (gptp_state->gm_is_present) {
        struct timeval timeout_time;
        gettimeofday(&timeout_time, NULL);
        gptp_gm_info_t *current_gm = get_current_gm();
        // Set timeout time to GM_TIMEOUT * GM's announce period (rounded down to whole seconds)
        timeout_time.tv_sec -= CONFIG_GM_TIMEOUT * (int)(current_gm->announce_period / 1000);
        // If current GM timed out
        if (compare_timeval(current_gm->last_announce, timeout_time) < 0) {
            // Remove it from the list of GMs; and next best GM becomes current GM at gm_list[0]
            esp_err_t err = remove_from_gm_list(current_gm);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to remove GM from list.");
            }
            // Get new current GM and reset state values
            current_gm = get_current_gm();
            gettimeofday(&current_gm->last_announce, NULL);
            current_gm->announce_period = CONFIG_GM_ANNOUNCE_PERIOD;
            current_gm->announce_sequence_id = 0;
            current_gm->sync_period = CONFIG_GM_SYNC_PERIOD;
            current_gm->sync_sequence_id = 0;

            // If new current GM is the local clock, then set GM is local
            if (current_gm->clock_id == CONFIG_CLOCK_ID) {
                gptp_state->gm_is_local = true;
            }
        }
    }
    // If GM is not present and not local (startup case)
    else {
        // Local clock is already set to gm_list[0] by default
        // So just reset the GM last_announce time and the gPTP state variables
        gettimeofday(&current_gm->last_announce, NULL);
        gptp_state->gm_is_present = true;
        gptp_state->gm_is_local = true;
    }
}

// Send gPTP peer delay request message
static void send_gptp_pdelay_request(void* arg) {

    // Increment the sequence id from gPTP state
    gptp_state_info_t *gptp_state = get_gptp_state();
    if (gptp_state->pdelay_request_sequence_id == 0xFFFF) { gptp_state->pdelay_request_sequence_id = 1; }
    else { gptp_state->pdelay_request_sequence_id += 1; }

    // Create a new frame, set the header and append the request message
    eth_frame_t frame;
    set_frame_header(lldp_mcast_mac_addr, ethertype_gptp, &frame);
    frame.frame_type = avb_frame_gptp_pdelay_request;
    append_gptpdu(frame.frame_type, &frame);

    // Insert the sequence_id
    int_to_octets(&gptp_state->pdelay_request_sequence_id, frame.payload + 30, 2);

    // Send response
    send_frame(&frame);
    print_frame(&frame, PRINT_SUMMARY);

    // Save the request sent timestamp from the frame that was just sent
    gptp_pdelay_info_t *current_pdelay = get_current_pdelay();
    current_pdelay->timestamp_request_transmission.tv_sec = frame.time_sent.tv_sec;
    current_pdelay->timestamp_request_transmission.tv_usec = frame.time_sent.tv_usec;
}

// Send gPTP peer delay response and follow up messages
static void send_gptp_pdelay_response(eth_frame_t * req_frame) {

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
    print_frame(&frame, PRINT_SUMMARY);

    // --- Process response follow up based on request and response frame data ---
    
    // Reuse the frame; change payload to response follow up
    // Re-insert the sequence_id, requesting_source_port_identity, and requesting_source_port_id
    frame.frame_type = avb_frame_gptp_pdelay_response_follow_up;
    append_gptpdu(frame.frame_type, &frame);
    memcpy(frame.payload + 30, req_frame->payload + 30, (2));
    memcpy(frame.payload + 44, req_frame->payload + 20, (8));
    memcpy(frame.payload + 52, req_frame->payload + 28, (2));

    // Insert the response_origin_timestamp from the response time of transmission
    // using the time_sent from the response that was just sent
    timeval_to_octets(&frame.time_sent, frame.payload + 34, frame.payload + 40);
    
    // Send response follow up
    send_frame(&frame);
    print_frame(&frame, PRINT_SUMMARY);
}

// Send gPTP announce message (if GM is local)
static void send_gptp_announce(void* arg) {

    // Increment the sequence id from current GM
    gptp_gm_info_t *current_gm = get_current_gm();
    if (current_gm->announce_sequence_id == 0xFFFF) { current_gm->announce_sequence_id = 1; }
    else { current_gm->announce_sequence_id += 1; }

    // Create a new frame, set the header and append the announce message
    eth_frame_t frame;
    set_frame_header(lldp_mcast_mac_addr, ethertype_gptp, &frame);
    frame.frame_type = avb_frame_gptp_announce;
    append_gptpdu(frame.frame_type, &frame);

    // Insert the sequence_id
    int_to_octets(&current_gm->announce_sequence_id, frame.payload + 30, 2);

    // Insert the local gm info
    int_to_octets(&current_gm->priority_1, frame.payload + 47, 1);
    int_to_octets(&current_gm->clock_class, frame.payload + 48, 1);
    int_to_octets(&current_gm->clock_accuracy, frame.payload + 49, 1);
    int_to_octets(&current_gm->clock_variance, frame.payload + 50, 2);
    int_to_octets(&current_gm->priority_2, frame.payload + 52, 1);
    int_to_octets(&current_gm->clock_id, frame.payload + 53, 8);
    int_to_octets(&current_gm->steps_removed, frame.payload + 61, 2);
    int_to_octets(&current_gm->time_source, frame.payload + 63, 1);
    int_to_octets(&current_gm->clock_id, frame.payload + 68, 8); // pathTraceTlv(pathSequence)

    // Send the announce message
    //send_frame(&frame);
    //print_frame(&frame, PRINT_VERBOSE);
}

// Send gPTP sync message
static void send_gptp_sync(void* arg) {

    // If GM is present, local, and time is set  
    gptp_state_info_t *gptp_state = get_gptp_state();
    if (gptp_state->gm_is_present && gptp_state->gm_is_local && gptp_state->time_is_set) {

        // Increment the sequence id from current GM
        gptp_gm_info_t *current_gm = get_current_gm();
        if (current_gm->sync_sequence_id == 0xFFFF) { current_gm->sync_sequence_id = 1; }
        else { current_gm->sync_sequence_id += 1; }

        // Setup and send the sync message, and follow up message
    }
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
        if (get_frame(ethertype_avtp, &frame) == ESP_FAIL) {
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
        if (get_frame(ethertype_msrp, &frame) == ESP_FAIL) {
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
        if (get_frame(ethertype_mvrp, &frame) == ESP_FAIL) {
            break;
        }
        // Log the received frame
        print_frame(&frame, PRINT_SUMMARY);

        // Do something with the frame
    }
error:
    vTaskDelete(NULL);
}

// Starup the talker
void start_talker(esp_netif_iodriver_handle handle) {
    
    ESP_LOGI(TAG, "Starting Talker...");
    
    // Set the Ethernet driver handle (passed from main app)
    set_eth_handle(handle);

    // Open and configure L2 TAP file descriptors
    esp_err_t err = init_all_l2tap_fds();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize FDs, %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "All FDs are open.");
        
        // Start gPTP manager task
        xTaskCreate(gptp_task, "gptp_task", 6144, NULL, 5, NULL);

        // Start AVTP manager task
        //xTaskCreate(avtp_task, "avtp_task", 6144, NULL, 5, NULL);

        // Start MSRP manager task
        //xTaskCreate(msrp_task, "msrp_task", 6144, NULL, 5, NULL);

        // Start MVRP manager task
        //xTaskCreate(mvrp_task, "mvrp_task", 6144, NULL, 5, NULL);
    }
}

// Stop the talker
void stop_talker() {
    ESP_LOGI(TAG, "Stopping Talker...");
    close_all_l2tap_fds();
    ESP_LOGI(TAG, "All FDs are closed.");
}