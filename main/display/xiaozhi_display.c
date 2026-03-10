#include "xiaozhi_display.h"
#include "esp_lvgl_port.h"
#include "xiaozhi_lcd.h"

#include "esp_log.h"
#include <stdbool.h>

#define TAG "xiaozhi_display"
lv_obj_t *qr;
#define LCD_H_RES 320
#define LCD_V_RES 240
#define LCD_DRAW_BUFF_HEIGHT 10

static lv_display_t *lvgl_disp = NULL;
static bool s_lvgl_ready = false;

esp_err_t xiaozhi_display_init(void)
{
    ESP_RETURN_ON_ERROR(xiaozhi_lcd_init(), TAG, "lcd init failed");

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_priority = 2;
    lvgl_cfg.task_stack = 8192;
    lvgl_cfg.task_affinity = -1;
    lvgl_cfg.task_max_sleep_ms = 100;
    lvgl_cfg.timer_period_ms = 5;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port init failed");

    const uint32_t buff_size = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT;

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io_handle,
        .panel_handle = panel_handle,
        .buffer_size = buff_size,
        .double_buffer = true,
        .hres = LCD_V_RES,
        .vres = LCD_H_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .full_refresh = false,
            .direct_mode = false,
            .swap_bytes = true,
        },
    };

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(lvgl_disp != NULL, ESP_FAIL, TAG, "lvgl display add failed");
    s_lvgl_ready = true;
    return ESP_OK;
}

void xiaozhi_display_test(void)
{
    if (!s_lvgl_ready) {
        ESP_LOGW(TAG, "lvgl not ready");
        return;
    }

    if (!lvgl_port_lock(1000)) {
        ESP_LOGE(TAG, "failed to lock lvgl");
        return;
    }

    lv_obj_t *screen = lv_screen_active();
    if (screen == NULL) {
        ESP_LOGE(TAG, "active screen is null");
        lvgl_port_unlock();
        return;
    }

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), LV_PART_MAIN);

    lv_obj_t *text_label = lv_label_create(screen);
    lv_label_set_text(text_label, "Hello World!");
    lv_obj_set_style_bg_color(text_label, lv_color_hex(0xFBC778), LV_PART_MAIN);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(text_label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(text_label, LV_ALIGN_CENTER, 0, 0);
    lvgl_port_unlock();
}
void xiaozhi_display_show_qrcode(char *data, uint32_t data_len)
{
    if (!s_lvgl_ready || data == NULL || data_len == 0) {
        ESP_LOGW(TAG, "skip qrcode draw, lvgl not ready or payload invalid");
        return;
    }

    lvgl_port_lock(0);

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);

    qr = lv_qrcode_create(lv_screen_active());
    lv_qrcode_set_size(qr, 150);
    lv_qrcode_set_dark_color(qr, fg_color);
    lv_qrcode_set_light_color(qr, bg_color);

    /*Set data*/

    lv_qrcode_update(qr, data, data_len);
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);

    lvgl_port_unlock();
}

void xiaozhi_display_delete_qrcode(void)
{
    if (!s_lvgl_ready) {
        return;
    }
    if (qr == NULL)
    {
        return;
    }
    lvgl_port_lock(0);
    lv_obj_del(qr);
    qr = NULL;
    lvgl_port_unlock();
}
