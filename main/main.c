#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi.h"
#include "xiaozhi_display.h"

void button_cb(void *button_handle, void *usr_data)
{
    uint8_t event = (uint8_t)usr_data;
    if (event == 1) {
        ESP_LOGI(TAG, "sw2 signal pressed");
    } else if (event == 2) {
        ESP_LOGI(TAG, "sw3 signal pressed");
    } else if (event == 3) {
        ESP_LOGI(TAG, "sw3 long pressed");
        xiaozhi_wifi_reset_provisioning();
    }
}

void app_main(void)
{
    xiaozhi_button_init();
    xiaozhi_button_front_register_cb(BUTTON_SINGLE_CLICK, NULL, button_cb, (void *)1);
    xiaozhi_button_back_register_cb(BUTTON_SINGLE_CLICK, NULL, button_cb, (void *)2);
    xiaozhi_button_back_register_cb(BUTTON_LONG_PRESS_START, NULL, button_cb, (void *)3);

    // Display/LVGL must be initialized before wifi provisioning qrcode is shown.
    esp_err_t disp_ret = xiaozhi_display_init();
    if (disp_ret != ESP_OK) {
        ESP_LOGE("MAIN", "display init failed: %s", esp_err_to_name(disp_ret));
        return;
    }

    xiaozhi_wifi_init();
}
