#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdint.h>
#include <IPAddress.h>

void ntp_setup();
uint64_t getEpochTime();

#endif /* NTP_CLIENT_H */