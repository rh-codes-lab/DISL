#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"
#include "appfilesystem.h"
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <esp_spiffs.h>
#include "arty_driver.h"

#define DAEMON_TASK_PRIORITY    2
#define ARTY_TASK_PRIORITY     3 

extern void arty_driver_task(void *arg);

static const char *TAG = "appusbhost";

static void host_lib_daemon_task(void *arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

    ESP_LOGI(TAG, "Installing USB Host Library");
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    //Signal to the class driver task that the host library is installed
    xSemaphoreGive(signaling_sem);
    vTaskDelay(10); //Short delay to let client task spin up

    bool has_clients = true;
    bool has_devices = true;
    while (has_clients || has_devices ) 
    {
        uint32_t event_flags;
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
    }
    ESP_LOGI(TAG, "No more clients and devices");

    //Uninstall the USB Host Library
    ESP_ERROR_CHECK(usb_host_uninstall());
    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}

void init_usbhost(void)
{
    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

    TaskHandle_t daemon_task_hdl;
    TaskHandle_t arty_driver_task_hdl;
    //Create daemon task
    xTaskCreatePinnedToCore(host_lib_daemon_task,
                            "usbhdaemon",
                            4096,
                            (void *)signaling_sem,
                            DAEMON_TASK_PRIORITY,
                            &daemon_task_hdl,
                            0);
    //Create the class driver task
    xTaskCreatePinnedToCore(arty_driver_task,
                            "arty",
                            8192,
                            (void *)signaling_sem,
                            ARTY_TASK_PRIORITY,
                            &arty_driver_task_hdl,
                            0);

}
