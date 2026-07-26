#include "ui_hardware_monitor.h"
#include "ui.h"
#include <stdio.h>

static lv_obj_t *screen;
static lv_obj_t *cpu_value;
static lv_obj_t *gpu_value;
static lv_obj_t *ram_value;
static lv_obj_t *temp_value;
static lv_obj_t *net_value;
static lv_obj_t *battery_value;

static lv_color_t status_color(float value) {
    if (value < 50.0f) return lv_color_hex(0x66FF99);
    if (value < 75.0f) return lv_color_hex(0xFFE066);
    return lv_color_hex(0xFF6B6B);
}

static lv_obj_t *label(const char *text, int x, int y, lv_color_t color) {
    lv_obj_t *object = lv_label_create(screen);
    lv_obj_set_pos(object, x, y);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_font(object, &lv_font_vt323_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(object, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    return object;
}

void ui_hardware_monitor_init(void) {
    const lv_color_t label_color = lv_color_hex(0x46D977);
    const lv_color_t value_color = lv_color_hex(0xB6FFC9);

    screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x030804), LV_PART_MAIN | LV_STATE_DEFAULT);

    label("[ SYSTEM MONITOR ]", 10, 1, label_color);
    label("CPU", 10, 27, label_color);
    cpu_value = label("--", 65, 27, value_color);
    label("GPU", 10, 53, label_color);
    gpu_value = label("0.0%", 65, 53, value_color);
    label("RAM", 10, 79, label_color);
    ram_value = label("--", 65, 79, value_color);
    label("TMP", 10, 105, label_color);
    temp_value = label("--", 65, 105, value_color);
    label("NET", 10, 131, label_color);
    net_value = label("--", 65, 131, value_color);
    label("BAT", 10, 157, label_color);
    battery_value = label("--", 65, 157, value_color);

    lv_disp_load_scr(screen);
}

void ui_update_cpu(float percent, float freq_ghz) {
    char text[28];
    if (freq_ghz > 0.0f) snprintf(text, sizeof(text), "%4.1f%%  %3.1fGHz", percent, freq_ghz);
    else snprintf(text, sizeof(text), "%4.1f%%", percent);
    lv_label_set_text(cpu_value, text);
    lv_obj_set_style_text_color(cpu_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_gpu(float percent) {
    if (percent <= 0.0f) {
        lv_label_set_text(gpu_value, "0.0%");
        lv_obj_set_style_text_color(gpu_value, status_color(0.0f), LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }

    char text[16];
    snprintf(text, sizeof(text), "%4.1f%%", percent);
    lv_label_set_text(gpu_value, text);
    lv_obj_set_style_text_color(gpu_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_ram(float percent, float used_gb, float total_gb) {
    char text[28];
    if (used_gb > 0.0f && total_gb > 0.0f) snprintf(text, sizeof(text), "%4.1f%%  %4.1f/%4.1fG", percent, used_gb, total_gb);
    else snprintf(text, sizeof(text), "%4.1f%%", percent);
    lv_label_set_text(ram_value, text);
    lv_obj_set_style_text_color(ram_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_temp(float celsius, int fan_rpm) {
    char text[28];
    if (fan_rpm > 0) snprintf(text, sizeof(text), "%3.0fC  %4dRPM", celsius, fan_rpm);
    else snprintf(text, sizeof(text), "%3.0fC", celsius);
    lv_label_set_text(temp_value, text);
    lv_obj_set_style_text_color(temp_value, status_color((celsius - 30.0f) / 60.0f * 100.0f), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_network(float download_mbps, float upload_mbps) {
    char text[28];
    float down = download_mbps * 1024.0f;
    float up = upload_mbps * 1024.0f;
    if (download_mbps < 1.0f && upload_mbps < 1.0f) snprintf(text, sizeof(text), "D%4.0fk U%4.0fk", down, up);
    else if (download_mbps < 1.0f) snprintf(text, sizeof(text), "D%4.0fk U%3.1fM", down, upload_mbps);
    else if (upload_mbps < 1.0f) snprintf(text, sizeof(text), "D%3.1fM U%4.0fk", download_mbps, up);
    else snprintf(text, sizeof(text), "D%3.1fM U%3.1fM", download_mbps, upload_mbps);
    lv_label_set_text(net_value, text);
    lv_obj_set_style_text_color(net_value, status_color(download_mbps + upload_mbps), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_battery(int percent, float power_watts) {
    char text[20];
    if (percent < 0) {
        lv_label_set_text(battery_value, "N/A");
        lv_obj_set_style_text_color(battery_value, lv_color_hex(0x6B8A70), LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }

    snprintf(text, sizeof(text), "%3d%%  %3.1fW", percent, power_watts);
    lv_label_set_text(battery_value, text);
    lv_obj_set_style_text_color(battery_value, status_color(100.0f - percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}
