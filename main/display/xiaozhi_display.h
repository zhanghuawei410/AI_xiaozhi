#pragma once

#include "esp_err.h"

esp_err_t xiaozhi_display_init(void);

void xiaozhi_display_test(void);

void xiaozhi_display_show_qrcode(char *data, uint32_t data_len);

void xiaozhi_display_delete_qrcode(void);
