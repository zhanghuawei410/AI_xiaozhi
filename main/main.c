#include <stdio.h>
#include "esp_log.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi.h"

void button_cb(void *button_handle, void *usr_data)
{
    uint8_t event = (uint8_t)usr_data;
    if (event == 1)
    {
        ESP_LOGI(TAG,"sw2 signal pressed");
    }else if (event == 2)
    {
        ESP_LOGI(TAG,"sw3 signal pressed");
    }else if (event == 3)
    {
        ESP_LOGI(TAG,"sw3 long pressed");
        xiaozhi_wifi_reset_provisioning();
    }
}

void app_main(void)
{
    xiaozhi_button_init();
    //1. 注册按键回调
    xiaozhi_button_front_register_cb(BUTTON_SINGLE_CLICK, NULL, button_cb, (void *)1);
    xiaozhi_button_back_register_cb(BUTTON_SINGLE_CLICK, NULL, button_cb, (void *)2);
    xiaozhi_button_back_register_cb(BUTTON_LONG_PRESS_START, NULL, button_cb, (void *)3);
    ESP_LOGI("MAIN", "Initializing WS2812...");
    // wifi 初始化
    xiaozhi_wifi_init();
}