#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdint.h>
#include <IPAddress.h>

void ntp_setup();
uint64_t getEpochTime();
extern bool is_root; // Extern from main.cpp to determine if this node is root or child for NTP logic.
#endif /* NTP_CLIENT_H */