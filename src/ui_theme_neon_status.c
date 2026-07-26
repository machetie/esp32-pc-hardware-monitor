#include "ui_hardware_monitor.h"
#include "ui.h"
#include <stdio.h>

static lv_obj_t *screen;
static lv_obj_t *cpu_prefix;
static lv_obj_t *cpu_value;
static lv_obj_t *gpu_prefix;
static lv_obj_t *gpu_value;
static lv_obj_t *ram_prefix;
static lv_obj_t *ram_value;
static lv_obj_t *temp_prefix;
static lv_obj_t *temp_value;
static lv_obj_t *net_prefix;
static lv_obj_t *net_value;
static lv_obj_t *battery_icon;
static lv_obj_t *battery_value;

static lv_color_t status_color(float pct) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    if (pct < 25.0f) {
        uint8_t ratio = (uint8_t)(pct / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(0, 255, 0), lv_color_make(0, 255, 255), ratio);
    } else if (pct < 50.0f) {
        uint8_t ratio = (uint8_t)((pct - 25.0f) / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 255, 0), lv_color_make(0, 255, 0), ratio);
    } else if (pct < 75.0f) {
        uint8_t ratio = (uint8_t)((pct - 50.0f) / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 165, 0), lv_color_make(255, 255, 0), ratio);
    }

    uint8_t ratio = (uint8_t)((pct - 75.0f) / 25.0f * 255.0f);
    return lv_color_mix(lv_color_make(255, 0, 0), lv_color_make(255, 165, 0), ratio);
}

static lv_obj_t *create_label(const char *text, int x, int y, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *object = lv_label_create(screen);
    lv_obj_set_pos(object, x, y);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_font(object, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(object, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    return object;
}

static void create_divider(int y) {
    lv_obj_t *object = lv_obj_create(screen);
    lv_obj_set_pos(object, 5, y);
    lv_obj_set_size(object, 162, 1);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(object, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(object, lv_color_hex(0xFF3DF5), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_hardware_monitor_init(void) {
    const lv_color_t magenta = lv_color_hex(0xFF3DF5);
    const lv_color_t cyan = lv_color_hex(0x33E7FF);

    screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x08051A), LV_PART_MAIN | LV_STATE_DEFAULT);

    gpu_prefix = create_label(LV_SYMBOL_IMAGE, 10, 5, &lv_font_montserrat_30, magenta);
    gpu_value = create_label("0.0%", 60, 5, &lv_font_montserrat_32, cyan);
    create_divider(34);

    cpu_prefix = create_label(LV_SYMBOL_SETTINGS, 10, 38, &lv_font_montserrat_30, magenta);
    cpu_value = create_label("0.0%", 60, 38, &lv_font_montserrat_32, cyan);
    create_divider(67);

    ram_prefix = create_label(LV_SYMBOL_SD_CARD, 10, 71, &lv_font_montserrat_30, magenta);
    ram_value = create_label("0%", 60, 71, &lv_font_montserrat_32, cyan);
    create_divider(100);

    temp_prefix = create_label(LV_SYMBOL_TINT, 10, 104, &lv_font_montserrat_30, magenta);
    temp_value = create_label("0°C", 60, 104, &lv_font_montserrat_32, cyan);
    create_divider(133);

    net_prefix = create_label(LV_SYMBOL_WIFI, 10, 137, &lv_font_montserrat_30, magenta);
    net_value = create_label("(not available)", 60, 137, &lv_font_montserrat_32, cyan);

    battery_icon = lv_label_create(screen);
    lv_obj_align(battery_icon, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(battery_icon, magenta, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(battery_icon, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    battery_value = lv_label_create(screen);
    lv_obj_align_to(battery_value, battery_icon, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_label_set_text(battery_value, "");
    lv_obj_set_style_text_color(battery_value, cyan, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(battery_value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_disp_load_scr(screen);
}

void ui_update_cpu(float percent, float freq_ghz) {
    char text[48];
    if (freq_ghz > 0.0f) snprintf(text, sizeof(text), "%.1f%% %.1fGHz", percent, freq_ghz);
    else snprintf(text, sizeof(text), "%.1f%%", percent);
    lv_label_set_text(cpu_value, text);
    lv_obj_set_style_text_color(cpu_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_gpu(float percent) {
    if (percent > 0.0f) {
        char text[32];
        snprintf(text, sizeof(text), "%.1f%%", percent);
        lv_label_set_text(gpu_value, text);
        lv_obj_set_style_text_color(gpu_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_label_set_text(gpu_value, "0.0%");
        lv_obj_set_style_text_color(gpu_value, status_color(0.0f), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_update_ram(float percent, float used_gb, float total_gb) {
    char text[48];
    if (used_gb > 0.0f && total_gb > 0.0f) snprintf(text, sizeof(text), "%.0f%% %.1f/%.1fGB", percent, used_gb, total_gb);
    else snprintf(text, sizeof(text), "%.0f%%", percent);
    lv_label_set_text(ram_value, text);
    lv_obj_set_style_text_color(ram_value, status_color(percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_temp(float celsius, int fan_rpm) {
    char text[48];
    if (fan_rpm > 0) snprintf(text, sizeof(text), "%.0f°C %dRPM", celsius, fan_rpm);
    else snprintf(text, sizeof(text), "%.0f°C", celsius);
    lv_label_set_text(temp_value, text);
    lv_obj_set_style_text_color(temp_value, status_color((celsius - 30.0f) / 60.0f * 100.0f), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_network(float download_mbps, float upload_mbps) {
    char text[64];
    float download_kbps = download_mbps * 1024.0f;
    float upload_kbps = upload_mbps * 1024.0f;
    if (download_mbps < 1.0f && upload_mbps < 1.0f) snprintf(text, sizeof(text), "%s%.0fk %s%.0fk", LV_SYMBOL_DOWN, download_kbps, LV_SYMBOL_UP, upload_kbps);
    else if (download_mbps < 1.0f) snprintf(text, sizeof(text), "%s%.0fk %s%.1fM", LV_SYMBOL_DOWN, download_kbps, LV_SYMBOL_UP, upload_mbps);
    else if (upload_mbps < 1.0f) snprintf(text, sizeof(text), "%s%.1fM %s%.0fk", LV_SYMBOL_DOWN, download_mbps, LV_SYMBOL_UP, upload_kbps);
    else snprintf(text, sizeof(text), "%s%.1fM %s%.1fM", LV_SYMBOL_DOWN, download_mbps, LV_SYMBOL_UP, upload_mbps);

    float total_speed = download_mbps + upload_mbps;
    lv_label_set_text(net_value, text);
    if (total_speed < 0.1f) lv_obj_set_style_text_color(net_value, lv_color_hex(0x7251B5), LV_PART_MAIN | LV_STATE_DEFAULT);
    else lv_obj_set_style_text_color(net_value, status_color(total_speed > 100.0f ? 100.0f : total_speed), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_battery(int percent, float power_watts) {
    char text[16];
    if (percent >= 0) {
        snprintf(text, sizeof(text), "%d%%", percent);
        lv_color_t color = status_color(100.0f - (float)percent);
        lv_label_set_text(battery_value, text);
        lv_obj_set_style_text_color(battery_icon, color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(battery_value, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_label_set_text(battery_value, "--%%");
        lv_obj_set_style_text_color(battery_icon, lv_color_hex(0x7251B5), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(battery_value, lv_color_hex(0x7251B5), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
