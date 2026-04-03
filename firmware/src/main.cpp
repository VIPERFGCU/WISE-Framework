#include "sdkconfig.h"
#include <Arduino.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include "esp_crt_bundle.h"

#include "ntp_sensor_node.h"

void ota_task(void *pvParameter);
void broadcast_ota_to_mesh(void *arg);

// --- CONFIG ---
#define PI_IP_ADDRESS "10.42.0.1"
#define MQTT_BROKER_URL "mqtt://" PI_IP_ADDRESS ":1883"

// --- OTA Packet Def ---
#define OTA_CHUNK_SIZE 1024
typedef struct {
    uint8_t type;                   // 0=data, 1=OTA_START, 2=OTA_CHUNK, 3=OTA_END
    uint32_t total_size;            //Total size of firmware.
    uint32_t offset;                //Where this chunk belongs in the file.
    uint16_t data_len;              // How many bytets in this chunk.
    uint8_t data[OTA_CHUNK_SIZE];   // The firmware binary.
} mesh_ota_packet_t;

static const char *TAG = "mesh_main";
static bool is_root = false;
static bool mqtt_connected = false;
static esp_mqtt_client_handle_t mqtt_client = NULL;
char device_id[32]; 
static uint32_t packet_counter = 0; 

void setup_device_id() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "esp32_%02x%02x%02x", mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "DEVICE ID: %s", device_id);
}

int8_t get_rssi() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

// --- MQTT ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "[ROOT] MQTT Connected");
        mqtt_connected = true;
        // SUBSCRIBE to the OTA Command Topic
        esp_mqtt_client_subscribe(mqtt_client, "mesh/ota/command", 0);
        // Subscribe to the Propogate command.
        esp_mqtt_client_subscribe(mqtt_client, "mesh/ota/propagate", 0);
        break;
        
    case MQTT_EVENT_DATA:
        // Check if the message is for OTA
        if (strncmp(event->topic, "mesh/ota/command", event->topic_len) == 0) {
            char url[128];
            snprintf(url, event->data_len + 1, "%.*s", event->data_len, event->data);
            ESP_LOGW(TAG, "[OTA] Received Command! Downloading from: %s", url);
            // Start the Download Task
            xTaskCreate(ota_task, "ota_task", 8192, strdup(url), 5, NULL);
        }
            // Check for PROPAGATE Command for Child sensors.
        else if (strncmp(event->topic, "mesh/ota/propagate", event->topic_len) == 0) {
            char size_str[32] = {0};
            snprintf(size_str, event->data_len + 1, "%.*s", event->data_len, event->data);
            uint32_t fw_size = (uint32_t)atoi(size_str);
            
            if (fw_size == 0) {
                ESP_LOGE(TAG, "ERROR: Propagate command MUST include exact file size!");
                return;
            }
            
            ESP_LOGW(TAG, "[OTA] Received Propagate Command! Exact Size: %lu", fw_size);
            xTaskCreate(broadcast_ota_to_mesh, "mesh_broadcast", 8192, (void*)fw_size, 5, NULL);
        }
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "[ROOT] MQTT Disconnected");
        mqtt_connected = false;
        break;
    default: break;
    }
}

void start_mqtt() {
    if (mqtt_client != NULL) return; 
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.uri = MQTT_BROKER_URL;
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT Client Started... Waiting for connection...");
}

// --- PACKET SENDER ---
void send_mesh_packet(const char *json_payload) {
    if (is_root && mqtt_connected) {
        esp_mqtt_client_publish(mqtt_client, "mesh/data", json_payload, 0, 0, 0);
        ESP_LOGI(TAG, "ROOT TX: %s", json_payload);
    } else {
        mesh_data_t data;
        data.data = (uint8_t *)json_payload;
        data.size = strlen(json_payload);
        data.proto = MESH_PROTO_BIN;
        data.tos = MESH_TOS_P2P;
        esp_mesh_send(NULL, &data, 0, NULL, 0);
        ESP_LOGI(TAG, "NODE TX: Sent to Root");
    }
}

// --- TASKS ---
//Theoretical Implementation for receiver logic
void mesh_ota_receiver_logic(uint8_t *incoming_data, size_t len) {
    mesh_ota_packet_t *packet = (mesh_ota_packet_t *)incoming_data;
    static esp_ota_handle_t update_handle = 0;
    static const esp_partition_t *update_partition = NULL;
    
    if (packet->type == 1) { // OTA START
        ESP_LOGI(TAG, "OTA Start. Total Size: %lu", (unsigned long)packet->total_size);
        update_partition = esp_ota_get_next_update_partition(NULL);
        if (update_partition == NULL) {
            ESP_LOGE(TAG, "OTA Passive Partition not found.");
            return;
        }
        ESP_ERROR_CHECK(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle));
    } else if (packet->type == 2) { // OTA CHUNK
        if (update_handle) {
            //Write the chunk
            esp_ota_write(update_handle, packet->data, packet->data_len);
        }
    } else if (packet->type == 3) { // OTA END
        if (update_handle) {
            esp_ota_end(update_handle);
            esp_ota_set_boot_partition(update_partition);
            ESP_LOGI(TAG, "OTA Update Complete. Rebooting...");
            esp_restart();
        }
    }
}

// --- MESH RECEIVER (Root Only) ---
// Modifying for Child OTA.
void mesh_p2p_rx_task(void *arg) {
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = (uint8_t*)heap_caps_malloc(1500, MALLOC_CAP_8BIT);    //Buffer size.
    while (1) {
        data.size = 1500;
        // Listen for any packet.
        if (esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0) == ESP_OK) {
            
            //Peek at first byte to check type.
            uint8_t packet_type = data.data[0];

            if (packet_type >= 1 && packet_type <= 3){
                //It is an OTA packet
                //Pass to helper function
                mesh_ota_receiver_logic(data.data, data.size);
            }
            // ==== TIME SYNC( Yeah I could use a Semaphore but I can't be bothered to touch this code. ) ====
            else if (packet_type == MESH_PACKET_TIME_REQ && is_root) {
                // Root intercepts a time request from a child
                handle_mesh_time_request(&from);
            }
            else if (packet_type == MESH_PACKET_TIME_RES && !is_root) {
                // Child intercepts the time response it was waiting for
                mesh_time_res_t *res = (mesh_time_res_t *)data.data;
                received_mesh_time = res->current_time; // This unblocks getMeshTime()
                ESP_LOGI(TAG, "Node received mesh time: %ld", received_mesh_time);
            }
            // ---- TIME SYNC END ----
            else {
                // Sensor Data, forward to MQTT
                            data.data[data.size] = 0;
                if (is_root && mqtt_connected) {
                    esp_mqtt_client_publish(mqtt_client, "mesh/data", (char *)data.data, 0, 0, 0);
                    ESP_LOGI(TAG, "ROOT FWD: %s", (char *)data.data);
                } else {
                    ESP_LOGI(TAG, "RX Data: %s", (char *)data.data);
                }
            }
        }
    }
    vTaskDelete(NULL);

}


// --- IP EVENT HANDLER ---
// --- IP EVENT HANDLER ---
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, ">>> ROOT GOT IP: " IPSTR " <<<", IP2STR(&event->ip_info.ip));
        
        // Start MQTT
        if (is_root) {
            start_mqtt();
        }
    }
}
// --- MESH EVENT HANDLER ---
void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
    case MESH_EVENT_STARTED:
        ESP_LOGI(TAG, "MESH STARTED");
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        if (esp_mesh_is_root()) {
            ESP_LOGI(TAG, ">>> I AM ROOT <<<");
            is_root = true;

            // ESP-Mesh disables DHCP by default. Explicitly start for root.
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_dhcpc_start(netif);
                ESP_LOGI(TAG, "Requested DHCP IP from Pi WIFI Hotspot...");
            } else {
                ESP_LOGE(TAG, "Could not find WIFI interface!");
            }

            xTaskCreate(mesh_p2p_rx_task, "p2p_rx", 3072, NULL, 5, NULL);
        } else {
            is_root = false;
            ESP_LOGI(TAG, ">>> I AM NODE <<<");
            xTaskCreate(mesh_p2p_rx_task, "p2p_rx", 3072, NULL, 5, NULL);
        }
        
        ntp_sensor_node_init();
        break;
    case MESH_EVENT_PARENT_DISCONNECTED:
        if (is_root) { 
            esp_mqtt_client_stop(mqtt_client); 
            mqtt_connected = false;
            is_root = false;
        }
        esp_mesh_connect();
        break;
    default: break;
    }
}


void broadcast_ota_to_mesh(void *arg) {
    uint32_t exact_fw_size = (uint32_t)arg;
    char debug_msg[128];

    // 1. Notify MQTT we are starting
    snprintf(debug_msg, sizeof(debug_msg), "LOG: Starting OTA Broadcast Task...");
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", debug_msg, 0, 0, 0);

    // 2. Get Routing Table
    int route_table_size = esp_mesh_get_routing_table_size();
    snprintf(debug_msg, sizeof(debug_msg), "LOG: Routing Table Size: %d", route_table_size);
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", debug_msg, 0, 0, 0);

    if (route_table_size < 1) {
        esp_mqtt_client_publish(mqtt_client, "mesh/debug", "ERROR: No children found!", 0, 0, 0);
        vTaskDelete(NULL);
        return;
    }

    mesh_addr_t *route_table = (mesh_addr_t *)malloc(route_table_size * sizeof(mesh_addr_t));
    ESP_ERROR_CHECK(esp_mesh_get_routing_table(route_table, route_table_size * sizeof(mesh_addr_t), &route_table_size));

    const esp_partition_t *update_partition = esp_ota_get_running_partition();
    if (!update_partition) {
         vTaskDelete(NULL);
         return;
    }

    mesh_ota_packet_t packet;
    packet.type = 1; // OTA START
    packet.total_size = exact_fw_size; // Passing exact size for logging
    packet.offset = 0;

    uint8_t my_mac[6];
    esp_read_mac(my_mac, ESP_MAC_WIFI_STA);

    mesh_data_t mesh_data = {
        .data = (uint8_t *)&packet,
        .size = sizeof(packet),
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P,
    };

    // 4. Send START Command
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "LOG: Sending Start Packet...", 0, 0, 0);
    for (int i = 0; i < route_table_size; i++) {
        if (memcmp(route_table[i].addr, my_mac, 6) == 0) continue; 
        esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(3000)); 

    // 5. Send CHUNKS (Slow & Steady, No Retries)
    uint32_t offset = 0;
    packet.type = 2; // OTA CHUNK
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "LOG: Sending Data Chunks...", 0, 0, 0);

    while (offset < exact_fw_size) {
        uint32_t chunk_len = OTA_CHUNK_SIZE;
        
        if (offset + OTA_CHUNK_SIZE > exact_fw_size) {
            chunk_len = exact_fw_size - offset; 
        }

        esp_partition_read(update_partition, offset, packet.data, chunk_len);
        
        packet.offset = offset;
        packet.data_len = chunk_len; 
        mesh_data.size = sizeof(mesh_ota_packet_t); 

        for (int i = 0; i < route_table_size; i++) {
            if (memcmp(route_table[i].addr, my_mac, 6) == 0) continue;
            
            // Send EXACTLY once. Rely on MAC-layer reliability.
            esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
        }
        
        offset += chunk_len;
        
        // 400ms throttle. Ensures Child flash memory never falls behind.
        vTaskDelay(pdMS_TO_TICKS(400)); 
    }

    // 6. Send END Command
    packet.type = 3; // OTA END
    mesh_data.size = sizeof(packet);
    for (int i = 0; i < route_table_size; i++) {
        if (memcmp(route_table[i].addr, my_mac, 6) == 0) continue;
        esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
    }

    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "SUCCESS: Broadcast Complete!", 0, 0, 0);
    
    free(route_table);
    vTaskDelete(NULL);
}

void ota_task(void *pvParameter) {
    char *url = (char *)pvParameter;
    ESP_LOGI(TAG, "Starting OTA Download from: %s", url);

    // 1. Zero-initialize the struct to prevent all those compiler warnings
    esp_http_client_config_t config = {}; 
    
    // 2. Assign the variables directly (Standard C++ style)
    config.url = url;
    config.cert_pem = NULL; // No SSL for local testing
    config.timeout_ms = 5000;
    config.keep_alive_enable = true;

    // 3. Pass the HTTP config DIRECTLY to the OTA function.
    // Notice we completely removed the esp_https_ota_config_t wrapper.
    esp_err_t ret = esp_https_ota(&config);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Update Successful! Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Update Failed! Error: %d", ret);
    }
    
    vTaskDelete(NULL);
}

void setup(void) {
    // NVS Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Creating the interfaiio so "WIFI_STA_DEF" exists
    esp_netif_t *netif_sta = NULL;
    esp_netif_t *netif_ap = NULL;
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, &netif_ap));

    setup_device_id();

    // WIFI Init
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // Mesh Init
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));    // Listen for DHCP Assignment

    // Mesh Configuration
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memset(&cfg, 0, sizeof(mesh_cfg_t));

    uint8_t my_mesh_id[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 };
    memcpy((uint8_t *) &cfg.mesh_id, my_mesh_id, 6);
    
    cfg.channel = 0; 
    cfg.router.ssid_len = strlen("wise-gateway");
    memcpy((uint8_t *) &cfg.router.ssid, "wise-gateway", cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, "eaglemesh*", strlen("eaglemesh*"));
    
    cfg.mesh_ap.max_connection = 6;
    cfg.mesh_ap.nonmesh_max_connection = 4;
    memcpy((uint8_t *) &cfg.mesh_ap.password, "eaglemesh*", strlen("eaglemesh*"));

    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(TAG, "Mesh started successfully, waiting for root...");

    //OTA Test (Uncomment and build new firmware to send to root for OTA.)
    ESP_LOGI(TAG, "DEVICE ID: %s (v2.0.9 OTA SUCCESS)", device_id);
    //ESP_LOGI(TAG, "DEVICE ID: %s (v2.0.4)", device_id);

void loop() {};