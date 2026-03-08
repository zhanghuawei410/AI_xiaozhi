#include "xiaozhi_button.h"
#include "FreeRTOSConfig.h"

// create adc button
const button_config_t sw2_btn_cfg = {0};
const button_config_t sw3_btn_cfg = {0};
button_handle_t sw2_adc_btn = NULL;
button_handle_t sw3_adc_btn = NULL;


void xiaozhi_button_init(void)
{
    button_adc_config_t sw2_btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = 7,  
        .button_index = 0,
        .min = 0,
        .max = 500,
    };
    button_adc_config_t sw3_btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = 7,
        .button_index = 1,
        .min = 1500,
        .max = 2000,
    };
    esp_err_t ret = iot_button_new_adc_device(&sw2_btn_cfg, &sw2_btn_adc_cfg, &sw2_adc_btn);
    ret = iot_button_new_adc_device(&sw3_btn_cfg, &sw3_btn_adc_cfg, &sw3_adc_btn);

    if (NULL == sw2_adc_btn || NULL == sw3_adc_btn )
    {
        ESP_LOGE(TAG, "Button create failed");
    }
}
void xiaozhi_button_front_register_cb(button_event_t event, button_event_args_t *event_args, button_cb_t cb, void *usr_data)
{
    iot_button_register_cb(sw2_adc_btn, event, NULL, cb,usr_data);
}
void xiaozhi_button_back_register_cb(button_event_t event, button_event_args_t *event_args, button_cb_t cb, void *usr_data)
{
    iot_button_register_cb(sw3_adc_btn, event, NULL, cb,usr_data);
}
