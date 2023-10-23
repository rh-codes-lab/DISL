#pragma once
#ifdef __cplusplus
extern "C" {
#endif
unsigned getAPCount(void);
EventGroupHandle_t* getWifiEventGroup(void);
void establish_ssid_and_pw(void);
void scan(void);
esp_err_t connect_station(void);
void init_network(void);
#ifdef __cplusplus
}
#endif

