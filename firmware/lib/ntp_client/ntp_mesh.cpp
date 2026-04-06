#include "ntp_mesh.h"
#include <esp_log.h>

static const char *TAG = "ntp_mesh";

volatile time_t received_mesh_time = 0; // Shared flag to unblock the child

// --- CHILD LOGIC: Request Time ---
time_t getMeshTime() {
    received_mesh_time = 0; // Reset flag
    
    mesh_time_req_t req;
    req.type = MESH_PACKET_TIME_REQ;
    
    mesh_data_t req_data;
    req_data.data = (uint8_t*)&req;
    req_data.size = sizeof(req);
    req_data.proto = MESH_PROTO_BIN;
    req_data.tos = MESH_TOS_P2P;

    // Sending to NULL automatically routes the packet to the Root node
    if (esp_mesh_send(NULL, &req_data, 0, NULL, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send time request to root");
        return 0;
    }

    ESP_LOGI(TAG, "Requested time from Root. Waiting for response...");

    // Spin/wait up to 5 seconds for rx_task to intercept the response
    int timeout_ms = 5000;
    while (received_mesh_time == 0 && timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_ms -= 10;
    }

    if (received_mesh_time == 0) {
        ESP_LOGW(TAG, "Timeout waiting for time response from root");
    }

    return received_mesh_time;
}

// --- ROOT LOGIC: Respond with Time ---
void handle_mesh_time_request(mesh_addr_t *from) {
    mesh_time_res_t res;
    res.type = MESH_PACKET_TIME_RES;
    
    // getEpochTime() returns microseconds. Convert back to seconds for the response
    // res.current_time = (time_t)(getEpochTime() / 1000000ULL);  PRODUCTION

    res.current_time = (time_t)(mock_getEpochTime() / 1000000ULL); //MOCK

    mesh_data_t res_data;
    res_data.data = (uint8_t*)&res;
    res_data.size = sizeof(res);
    res_data.proto = MESH_PROTO_BIN;
    res_data.tos = MESH_TOS_P2P;

    esp_mesh_send(from, &res_data, 0, NULL, 0);
    ESP_LOGI(TAG, "Sent time response %ld back to child", res.current_time);
}