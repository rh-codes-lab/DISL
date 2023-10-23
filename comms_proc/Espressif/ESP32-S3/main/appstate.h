#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void read_wifi_config(void);
void write_wifi_config(void);
void init_time(void);
char* getHostname(void);
void setHostname(uint8_t* mac);
void setup_mdns(void);
#ifdef __cplusplus
}
#endif

