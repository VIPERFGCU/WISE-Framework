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
void ota_task(void *pvParameter);
void broadcast_ota_to_mesh(void *arg);

// --- CONFIG ---
#define PI_IP_ADDRESS "10.10.10.1"
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
static char device_id[32]; 
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
    esp_mqtt_event_handle_t event = event_data;
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
            ESP_LOGW(TAG, "[OTA] Received Propagate Command! Starting Mesh Broadcast...");
            // Running as a task to avoid blocking the MQTT handler
            xTaskCreate(broadcast_ota_to_mesh, "mesh_broadcast", 8192, NULL, 5, NULL);
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
    esp_mqtt_client_config_t mqtt_cfg = { .broker.address.uri = MQTT_BROKER_URL };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
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
void data_task(void *arg) {
    char payload[256];
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Slow down to 5s for debugging
        packet_counter++;
        snprintf(payload, sizeof(payload), 
                 "{\"id\":\"%s\",\"v\":2,\"type\":\"data\",\"msg_id\":%lu,\"val\":[%.2f,%.2f,%.2f]}", //need to fix this 'v' to be dynamically changed per firmware version.
                 device_id, (unsigned long)packet_counter,
                 (float)(esp_random() % 100) / 10.0,
                 (float)(esp_random() % 100) / 10.0,
                 (float)(esp_random() % 100) / 10.0);
        send_mesh_packet(payload);
    }
}


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
    data.data = heap_caps_malloc(1500, MALLOC_CAP_8BIT);    //Buffer size.
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

            // --- FORCE STATIC IP LOGIC ---
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                ESP_LOGW(TAG, "Found Interface! Stopping DHCP and Forcing IP...");
                esp_netif_dhcpc_stop(netif);
                
                esp_netif_ip_info_t ip_info;
                IP4_ADDR(&ip_info.ip, 10, 10, 10, 5);      // FORCE IP: 10.10.10.5
                IP4_ADDR(&ip_info.gw, 10, 10, 10, 1);      // GATEWAY: 10.10.10.1
                IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
                
                esp_netif_set_ip_info(netif, &ip_info);
                ESP_LOGI(TAG, "Static IP Set to 10.10.10.5. Starting MQTT...");
                
                start_mqtt();
            } else {
                ESP_LOGE(TAG, "COULD NOT FIND WIFI INTERFACE HANDLE!");
            }
            // -----------------------------

            xTaskCreate(mesh_p2p_rx_task, "p2p_rx", 3072, NULL, 5, NULL);
        } else {
            is_root = false;
            ESP_LOGI(TAG, ">>> I AM NODE <<<");
            xTaskCreate(mesh_p2p_rx_task, "p2p_rx", 3072, NULL, 5, NULL);
        }
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


// Theoretical Root Broadcaster Implementation
void broadcast_ota_to_mesh(void *arg) {
    char debug_msg[128];

    // 1. Notify MQTT we are starting
    snprintf(debug_msg, sizeof(debug_msg), "LOG: Starting OTA Broadcast Task...");
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", debug_msg, 0, 0, 0);

    // 2. Get Routing Table
    int route_table_size = esp_mesh_get_routing_table_size();
    
    // DEBUG: Tell us how many nodes are found
    snprintf(debug_msg, sizeof(debug_msg), "LOG: Routing Table Size: %d", route_table_size);
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", debug_msg, 0, 0, 0);

    if (route_table_size < 1) {
        esp_mqtt_client_publish(mqtt_client, "mesh/debug", "ERROR: No children found! Aborting.", 0, 0, 0);
        vTaskDelete(NULL);
        return;
    }

    mesh_addr_t *route_table = (mesh_addr_t *)malloc(route_table_size * sizeof(mesh_addr_t));
    ESP_ERROR_CHECK(esp_mesh_get_routing_table(route_table, route_table_size * sizeof(mesh_addr_t), &route_table_size));

    // 3. Prepare Payload
    const esp_partition_t *update_partition = esp_ota_get_running_partition();
    if (!update_partition) {
         esp_mqtt_client_publish(mqtt_client, "mesh/debug", "ERROR: Could not find partition!", 0, 0, 0);
         vTaskDelete(NULL);
         return;
    }

    mesh_ota_packet_t packet;
    packet.type = 1; // OTA START
    packet.total_size = update_partition->size;
    packet.offset = 0;

    mesh_data_t mesh_data = {
        .data = (uint8_t *)&packet,
        .size = sizeof(packet),
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P,
    };

    // 4. Send START Command
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "LOG: Sending Start Packet...", 0, 0, 0);
    
    for (int i = 0; i < route_table_size; i++) {
        esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); 

    // 5. Send CHUNKS
    uint32_t offset = 0;
    packet.type = 2; // OTA CHUNK
    
    // Notify MQTT we are entering the loop
    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "LOG: Sending Data Chunks...", 0, 0, 0);

    while (offset < update_partition->size) {
        esp_partition_read(update_partition, offset, packet.data, OTA_CHUNK_SIZE);
        packet.offset = offset;
        packet.data_len = OTA_CHUNK_SIZE;
        mesh_data.size = sizeof(mesh_ota_packet_t);

        for (int i = 0; i < route_table_size; i++) {
            esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
        }
        
        offset += OTA_CHUNK_SIZE;
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }

    // 6. Send END Command
    packet.type = 3; // OTA END
    mesh_data.size = sizeof(packet);
    for (int i = 0; i < route_table_size; i++) {
        esp_mesh_send(&route_table[i], &mesh_data, MESH_DATA_P2P, NULL, 0);
    }

    esp_mqtt_client_publish(mqtt_client, "mesh/debug", "SUCCESS: Broadcast Complete!", 0, 0, 0);
    
    free(route_table);
    vTaskDelete(NULL);
}

void ota_task(void *pvParameter) {
    char *url = (char *)pvParameter;
    ESP_LOGI(TAG, "Starting OTA Download from: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL, // No SSL for local testing
        .timeout_ms = 5000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Update Successful! Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Update Failed! Error: %d", ret);
    }
    vTaskDelete(NULL);
}

void app_main(void) {
    // NVS Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Creating the interfaces so "WIFI_STA_DEF" exists
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
    ESP_LOGI(TAG, "DEVICE ID: %s (v2.0 OTA SUCCESS)", device_id);
    //ESP_LOGI(TAG, "DEVICE ID: %s (v1.0)", device_id);

    xTaskCreate(data_task, "data_task", 3072, NULL, 5, NULL);
}