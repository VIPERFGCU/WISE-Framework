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
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "[ROOT] MQTT Disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "[ROOT] MQTT Error");
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
                 "{\"id\":\"%s\",\"type\":\"data\",\"msg_id\":%lu,\"val\":[%.2f,%.2f,%.2f]}", 
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
        ESP_LOGI(TAG, "OTA Start. Total Size: %lu", (unbsigned long)packet->total_size);
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
            uint8_t packet-type = data.data[0];

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
/* 
Called by root after it finishes its own OTA download.
It reads the firmware from the partition it just wrote to and broadcasts it.
*/
void broadcast_ota_to_mesh() {
    ESP_LOGI(TAG, "Starting Mesh OTA Distribution...");
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    //Or we send the partition we are currently running on.
    //const esp_partition_t *update_partition = esp_ota_get_running_partition();
    
    mesh_ota_packet_t packet;
    packet.type = 1; //OTA START
    packet.total_size = update_partition->size; // Size of the partition/file
    packet.offset = 0;
    
    //1. Send Start Command
    mesh_data_t mesh_data = {
        .data = (uint8_t *)&packet,
        .size = sizeof(packet),
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P,
    };

    // Use esp_mesh_send with NULL address for BROADCAST --- Send to all children.
    esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P | MESH_DATA_TODS, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));   //Delay for children to delete partitions.

    // 2. Loop through firmware and send chunks.
    uint32_t offset = 0;
    packet.type = 2; //OTA CHUNK

    while (offset < update_partition->size) {
        // Read flash
        esp_partition_read(update_partition, offset, packet.data, OTA_CHUNK_SIZE);
        packet.offset = offset;
        packet.data_len = OTA_CHUNK_SIZE; // Handle last chunk size logic

        mesh_data.size = sizeof(mesh_ota_packet_t);     // Update size
        
        //Send to mesh
        esp_err_t err = esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P | MESH_DATA_TODS, NULL, 0);

        if (err != ESP_OK) {
            //MESH can get congested.
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            offset += OTA_CHUNK_SIZE;
        }

        // An attempt to prevent mesh flooding
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    //3. Send End Command
    packet.type = 3; //OTA END
    mesh_data.size = sizeof(packet);
    esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P | MESH_DATA_TODS, NULL, 0);

    ESP_LOGI(TAG, "OTA Distribution Complete!");
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

    xTaskCreate(data_task, "data_task", 3072, NULL, 5, NULL);
}