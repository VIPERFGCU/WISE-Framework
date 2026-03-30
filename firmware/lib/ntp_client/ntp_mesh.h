#ifndef NTP_MESH_H
#define NTP_MESH_H

#include "ntp_client.h"
#include "esp_mesh.h"

// --- Custom Mesh Packet IDs ---
// Using 10 and 11 to keep them separated from OTA packet IDs (1, 2, 3)
#define MESH_PACKET_TIME_REQ 10
#define MESH_PACKET_TIME_RES 11

// --- Mesh Time Packet Structures ---
typedef struct __attribute__((packed)) {
    uint8_t type;
} mesh_time_req_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    time_t current_time;
} mesh_time_res_t;

// --- Shared Flag ---
// Expose this so main.cpp can unblock getMeshTime() when a response arrives
extern volatile time_t received_mesh_time; 

// --- Functions ---
time_t getMeshTime();
void handle_mesh_time_request(mesh_addr_t *from);

#endif // NTP_MESH_H