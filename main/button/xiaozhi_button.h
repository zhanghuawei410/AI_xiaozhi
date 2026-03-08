#pragma once
#include "esp_log.h"
#include "iot_button.h"
#include "button_adc.h"
#include "button_gpio.h"
#include "button_adc.h"
#define TAG "xiaozhi_button"

void xiaozhi_button_init(void);

void xiaozhi_button_front_register_cb(button_event_t event, button_event_args_t *event_args, button_cb_t cb, void *usr_data);

void xiaozhi_button_back_register_cb(button_event_t event, button_event_args_t *event_args, button_cb_t cb, void *usr_data);
