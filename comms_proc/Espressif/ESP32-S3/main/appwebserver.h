#pragma once
#if __cplusplus
extern "C" {
#endif
void stop_webserver(httpd_handle_t handle);
httpd_handle_t start_webserver(bool);
void create_ap(void);
#if __cplusplus
}
#endif


