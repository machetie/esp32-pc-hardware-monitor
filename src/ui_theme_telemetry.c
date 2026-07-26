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
static lv_obj_t *cpu_bar;
static lv_obj_t *gpu_bar;
static lv_obj_t *ram_bar;

static lv_color_t status_color(float value) {
    if (value < 50.0f) return lv_color_hex(0x00E5FF);
    if (value < 75.0f) return lv_color_hex(0xFFE066);
    return lv_color_hex(0xFF5A7A);
}

static lv_obj_t *label(const char *text, int x, int y, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *object = lv_label_create(screen);
    lv_obj_set_pos(object, x, y);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_font(object, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(object, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    return object;
}

static lv_obj_t *bar(int x, int y, int width) {
    lv_obj_t *object = lv_bar_create(screen);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, 6);
    lv_bar_set_range(object, 0, 100);
    lv_obj_set_style_radius(object, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(object, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(object, lv_color_hex(0x17213A), LV_PART_MAIN | LV_STATE_DEFAULT);
    return object;
}

static void set_bar(lv_obj_t *object, float value, lv_color_t color) {
    lv_bar_set_value(object, (int32_t)value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(object, color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

void ui_hardware_monitor_init(void) {
    const lv_color_t cyan = lv_color_hex(0x00E5FF);
    const lv_color_t purple = lv_color_hex(0xA855F7);
    const lv_color_t muted = lv_color_hex(0x8492B0);

    screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x060A16), LV_PART_MAIN | LV_STATE_DEFAULT);

    label("SYSTEM // LIVE", 10, 5, &lv_font_orbitron_16, purple);
    label("BAT", 248, 5, &lv_font_orbitron_16, purple);
    battery_value = label("--%", 285, 5, &lv_font_orbitron_16, cyan);

    label("GPU", 10, 29, &lv_font_orbitron_16, muted);
    gpu_value = label("0.0%", 10, 45, &lv_font_montserrat_32, cyan);
    gpu_bar = bar(10, 81, 142);

    label("CPU", 168, 29, &lv_font_orbitron_16, muted);
    cpu_value = label("--", 168, 45, &lv_font_montserrat_32, cyan);
    cpu_bar = bar(168, 81, 142);

    label("RAM", 10, 96, &lv_font_orbitron_16, purple);
    ram_value = label("--", 10, 112, &lv_font_montserrat_22, cyan);
    ram_bar = bar(10, 140, 142);

    label("TEMP // FAN", 168, 96, &lv_font_orbitron_16, purple);
    temp_value = label("--", 168, 112, &lv_font_montserrat_22, cyan);

    label("NET", 10, 151, &lv_font_orbitron_16, purple);
    net_value = label("D --  U --", 65, 151, &lv_font_montserrat_16, muted);

    lv_disp_load_scr(screen);
}

void ui_update_cpu(float percent, float freq_ghz) {
    char text[24];
    lv_color_t color = status_color(percent);
    if (freq_ghz > 0.0f) snprintf(text, sizeof(text), "%.0f%% %.1fG", percent, freq_ghz);
    else snprintf(text, sizeof(text), "%.0f%%", percent);
    lv_label_set_text(cpu_value, text);
    lv_obj_set_style_text_color(cpu_value, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    set_bar(cpu_bar, percent, color);
}

void ui_update_gpu(float percent) {
    if (percent <= 0.0f) {
        lv_label_set_text(gpu_value, "0.0%");
        lv_obj_set_style_text_color(gpu_value, status_color(0.0f), LV_PART_MAIN | LV_STATE_DEFAULT);
        set_bar(gpu_bar, 0.0f, lv_color_hex(0x00E5FF));
        return;
    }

    char text[12];
    lv_color_t color = status_color(percent);
    snprintf(text, sizeof(text), "%.0f%%", percent);
    lv_label_set_text(gpu_value, text);
    lv_obj_set_style_text_color(gpu_value, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    set_bar(gpu_bar, percent, color);
}

void ui_update_ram(float percent, float used_gb, float total_gb) {
    char text[28];
    lv_color_t color = status_color(percent);
    if (used_gb > 0.0f && total_gb > 0.0f) snprintf(text, sizeof(text), "%.0f%% %.1f/%.1fG", percent, used_gb, total_gb);
    else snprintf(text, sizeof(text), "%.0f%%", percent);
    lv_label_set_text(ram_value, text);
    lv_obj_set_style_text_color(ram_value, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    set_bar(ram_bar, percent, color);
}

void ui_update_temp(float celsius, int fan_rpm) {
    char text[28];
    lv_color_t color = status_color((celsius - 30.0f) / 60.0f * 100.0f);
    if (fan_rpm > 0) snprintf(text, sizeof(text), "%.0fC %d", celsius, fan_rpm);
    else snprintf(text, sizeof(text), "%.0fC --", celsius);
    lv_label_set_text(temp_value, text);
    lv_obj_set_style_text_color(temp_value, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_network(float download_mbps, float upload_mbps) {
    char text[32];
    float down = download_mbps * 1024.0f;
    float up = upload_mbps * 1024.0f;
    if (download_mbps < 1.0f && upload_mbps < 1.0f) snprintf(text, sizeof(text), "D %.0fk  U %.0fk", down, up);
    else if (download_mbps < 1.0f) snprintf(text, sizeof(text), "D %.0fk  U %.1fM", down, upload_mbps);
    else if (upload_mbps < 1.0f) snprintf(text, sizeof(text), "D %.1fM  U %.0fk", download_mbps, up);
    else snprintf(text, sizeof(text), "D %.1fM  U %.1fM", download_mbps, upload_mbps);
    lv_label_set_text(net_value, text);
    lv_obj_set_style_text_color(net_value, status_color(download_mbps + upload_mbps), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_battery(int percent, float power_watts) {
    char text[8];
    if (percent < 0) {
        lv_label_set_text(battery_value, "--");
        lv_obj_set_style_text_color(battery_value, lv_color_hex(0x8492B0), LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }

    snprintf(text, sizeof(text), "%d%%", percent);
    lv_label_set_text(battery_value, text);
    lv_obj_set_style_text_color(battery_value, status_color(100.0f - percent), LV_PART_MAIN | LV_STATE_DEFAULT);
}
