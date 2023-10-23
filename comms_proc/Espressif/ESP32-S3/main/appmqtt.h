#pragma once
#ifdef __cplusplus
extern "C" {
#endif
bool isMQTTRunning(void);
bool isMQTTConnected(void);
esp_mqtt_client_handle_t setup_mqtt(void);
void appmqtt_send_msg(char *topic, char* message);
void appmqtt_send_msg_n(char *topic, char* message, ssize_t n);
#ifdef __cplusplus
}
#endif
