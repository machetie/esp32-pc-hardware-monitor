#include "ui_hardware_monitor.h"
#include "ui.h"
#include <stdio.h>

// Helper: return a color on a green→yellow→red gradient based on a 0–100% value
// lv_color_mix(c1, c2, ratio): ratio=255 gives c1, ratio=0 gives c2
static lv_color_t get_pct_color(float pct) {
    // Clamp to valid range
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    // 0-25%: Cyan -> Green (Cool/Idle)
    if (pct < 25.0f) {
        uint8_t ratio = (uint8_t)(pct / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(0, 255, 0), lv_color_make(0, 255, 255), ratio);
    }
    // 25-50%: Green -> Yellow (Moderate)
    else if (pct < 50.0f) {
        uint8_t ratio = (uint8_t)((pct - 25.0f) / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 255, 0), lv_color_make(0, 255, 0), ratio);
    }
    // 50-75%: Yellow -> Orange (High)
    else if (pct < 75.0f) {
         uint8_t ratio = (uint8_t)((pct - 50.0f) / 25.0f * 255.0f);
         return lv_color_mix(lv_color_make(255, 165, 0), lv_color_make(255, 255, 0), ratio);
    }
    // 75-100%: Orange -> Red (Critical)
    else {
        uint8_t ratio = (uint8_t)((pct - 75.0f) / 25.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 0, 0), lv_color_make(255, 165, 0), ratio);
    }
}

// UI Elements
lv_obj_t * ui_HWMonScreen;

// Dual-label approach: Prefix (white) + Value (dynamic color)
lv_obj_t * ui_CPULabel_Prefix;
lv_obj_t * ui_CPULabel_Value;

lv_obj_t * ui_GPULabel_Prefix;
lv_obj_t * ui_GPULabel_Value;

lv_obj_t * ui_RAMLabel_Prefix;
lv_obj_t * ui_RAMLabel_Value;

lv_obj_t * ui_TempLabel_Prefix;
lv_obj_t * ui_TempLabel_Value;

lv_obj_t * ui_NetLabel_Prefix;
lv_obj_t * ui_NetLabel_Value;

lv_obj_t * ui_BatIcon;
lv_obj_t * ui_BatLabel_Value;

void ui_hardware_monitor_init(void) {
    // Create main screen
    ui_HWMonScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HWMonScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_HWMonScreen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);  // Black background

    // ========== DUAL-LABEL DISPLAY (6 LINES) ==========
    // Prefix labels: White text (static)
    // Value labels: Dynamic colored text
    // VT323 32pt font, Y=2 start, 28px spacing between lines

    // Line 1: GPU Information (Y=5)
    ui_GPULabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_GPULabel_Prefix, 10);
    lv_obj_set_y(ui_GPULabel_Prefix, 5);
    lv_label_set_text(ui_GPULabel_Prefix, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_color(ui_GPULabel_Prefix, lv_color_hex(0x9400D3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_GPULabel_Prefix, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_GPULabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_GPULabel_Value, 60);
    lv_obj_set_y(ui_GPULabel_Value, 5);
    lv_label_set_text(ui_GPULabel_Value, "0.0%");
    lv_obj_set_style_text_color(ui_GPULabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_GPULabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 2: CPU Information (Y=38)
    ui_CPULabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_CPULabel_Prefix, 10);
    lv_obj_set_y(ui_CPULabel_Prefix, 38);
    lv_label_set_text(ui_CPULabel_Prefix, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(ui_CPULabel_Prefix, lv_color_hex(0x9400D3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_CPULabel_Prefix, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CPULabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_CPULabel_Value, 60);
    lv_obj_set_y(ui_CPULabel_Value, 38);
    lv_label_set_text(ui_CPULabel_Value, "0.0%");
    lv_obj_set_style_text_color(ui_CPULabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_CPULabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 3: RAM Information (Y=71)
    ui_RAMLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_RAMLabel_Prefix, 10);
    lv_obj_set_y(ui_RAMLabel_Prefix, 71);
    lv_label_set_text(ui_RAMLabel_Prefix, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(ui_RAMLabel_Prefix, lv_color_hex(0x9400D3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_RAMLabel_Prefix, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_RAMLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_RAMLabel_Value, 60);
    lv_obj_set_y(ui_RAMLabel_Value, 71);
    lv_label_set_text(ui_RAMLabel_Value, "0%");
    lv_obj_set_style_text_color(ui_RAMLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_RAMLabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 4: Temperature Information (Y=104)
    ui_TempLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_TempLabel_Prefix, 10);
    lv_obj_set_y(ui_TempLabel_Prefix, 104);
    lv_label_set_text(ui_TempLabel_Prefix, LV_SYMBOL_TINT);
    lv_obj_set_style_text_color(ui_TempLabel_Prefix, lv_color_hex(0x9400D3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempLabel_Prefix, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TempLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_TempLabel_Value, 60);
    lv_obj_set_y(ui_TempLabel_Value, 104);
    lv_label_set_text(ui_TempLabel_Value, "0°C");
    lv_obj_set_style_text_color(ui_TempLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempLabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 5: Network Information (Y=137)
    ui_NetLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_NetLabel_Prefix, 10);
    lv_obj_set_y(ui_NetLabel_Prefix, 137);
    lv_label_set_text(ui_NetLabel_Prefix, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(ui_NetLabel_Prefix, lv_color_hex(0x9400D3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NetLabel_Prefix, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NetLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_NetLabel_Value, 60);
    lv_obj_set_y(ui_NetLabel_Value, 137);
    lv_label_set_text(ui_NetLabel_Value, "(not available)");
    lv_obj_set_style_text_color(ui_NetLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NetLabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 6: Battery Information (Top Right)
    ui_BatIcon = lv_label_create(ui_HWMonScreen);
    lv_obj_align(ui_BatIcon, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_label_set_text(ui_BatIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(ui_BatIcon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BatIcon, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BatLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_align_to(ui_BatLabel_Value, ui_BatIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_label_set_text(ui_BatLabel_Value, "");
    lv_obj_set_style_text_color(ui_BatLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BatLabel_Value, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Load the screen
    lv_disp_load_scr(ui_HWMonScreen);
}

void ui_update_cpu(float percent, float freq_ghz) {
    char text[48];

    // Display CPU percentage and frequency (if available) - value only
    if (freq_ghz > 0.0) {
        snprintf(text, sizeof(text), "%.1f%% %.1fGHz", percent, freq_ghz);
    } else {
        snprintf(text, sizeof(text), "%.1f%%", percent);
    }

    lv_label_set_text(ui_CPULabel_Value, text);

    // Set color based on CPU percentage (value label only)
    lv_color_t col = get_pct_color(percent);
    lv_obj_set_style_text_color(ui_CPULabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_gpu(float percent) {
    char text[32];

    // Display GPU percentage or unavailable message - value only
    if (percent > 0.0) {
        snprintf(text, sizeof(text), "%.1f%%", percent);
    } else {
        snprintf(text, sizeof(text), "(not available)");
    }

    lv_label_set_text(ui_GPULabel_Value, text);

    // Set color based on GPU percentage (value label only)
    if (percent > 0.0) {
        lv_color_t col = get_pct_color(percent);
        lv_obj_set_style_text_color(ui_GPULabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        // Gray color for unavailable
        lv_obj_set_style_text_color(ui_GPULabel_Value, lv_color_make(128, 128, 128), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_update_ram(float percent, float used_gb, float total_gb) {
    char text[48];
    // Display RAM percentage and memory usage - value only
    if (used_gb > 0.0 && total_gb > 0.0) {
        snprintf(text, sizeof(text), "%.0f%% %.1f/%.1fGB", percent, used_gb, total_gb);
    } else {
        snprintf(text, sizeof(text), "%.0f%%", percent);
    }
    lv_label_set_text(ui_RAMLabel_Value, text);

    // Set color based on RAM percentage (value label only)
    lv_color_t col = get_pct_color(percent);
    lv_obj_set_style_text_color(ui_RAMLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_temp(float celsius, int fan_rpm) {
    char text[48];
    // Display temperature and fan speed - value only
    if (fan_rpm > 0) {
        snprintf(text, sizeof(text), "%.0f°C %dRPM", celsius, fan_rpm);
    } else {
        snprintf(text, sizeof(text), "%.0f°C", celsius);
    }
    lv_label_set_text(ui_TempLabel_Value, text);

    // Color: Dynamic based on temperature
    // 30°C (0%) to 90°C (100%)
    float temp_pct = (celsius - 30.0f) / (90.0f - 30.0f) * 100.0f;
    lv_color_t col = get_pct_color(temp_pct);
    
    lv_obj_set_style_text_color(ui_TempLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_network(float download_mbps, float upload_mbps) {
    char text[64];
    const char *down_sym = LV_SYMBOL_DOWN;
    const char *up_sym = LV_SYMBOL_UP;
    
    // Convert MB/s to KB/s
    float download_kbps = download_mbps * 1024.0f;
    float upload_kbps = upload_mbps * 1024.0f;
    
    // Note: Arrows are not in Orbitron font, using D/U
    if (download_mbps < 1.0f && upload_mbps < 1.0f) {
        // Both speeds low - show in kB/s
        snprintf(text, sizeof(text), "%s%.0fk %s%.0fk", down_sym, download_kbps, up_sym, upload_kbps);
    } else if (download_mbps < 1.0f) {
        // Download low, upload high
        snprintf(text, sizeof(text), "%s%.0fk %s%.1fM", down_sym, download_kbps, up_sym, upload_mbps);
    } else if (upload_mbps < 1.0f) {
        // Download high, upload low
        snprintf(text, sizeof(text), "%s%.1fM %s%.0fk", down_sym, download_mbps, up_sym, upload_kbps);
    } else {
        // Both high - show in MB/s
        snprintf(text, sizeof(text), "%s%.1fM %s%.1fM", down_sym, download_mbps, up_sym, upload_mbps);
    }
    
    lv_label_set_text(ui_NetLabel_Value, text);

    // Set color based on total network speed (value label only)
    float total_speed = download_mbps + upload_mbps;
    lv_color_t col;
    if (total_speed < 0.1f) {
        // Idle/no activity - show white
        col = lv_color_make(255, 255, 255);
    } else if (total_speed < 60.0f) {
        // Low to moderate speed (0-60 MB/s) - green to yellow
        col = get_pct_color(total_speed);
    } else {
        // High speed (60-100+ MB/s) - yellow to red, clamped at 100
        col = get_pct_color(total_speed > 100.0f ? 100.0f : total_speed);
    }
    lv_obj_set_style_text_color(ui_NetLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_battery(int percent, float power_watts) {
    char text[16];
    // Display battery percentage - value only
    if (percent >= 0) {
        snprintf(text, sizeof(text), "%d%%", percent);
    } else {
        snprintf(text, sizeof(text), "--%%");
    }
    lv_label_set_text(ui_BatLabel_Value, text);
    lv_obj_align_to(ui_BatLabel_Value, ui_BatIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

    // Set icon based on percentage
    const char* symbol = LV_SYMBOL_BATTERY_EMPTY;
    if (percent > 90) symbol = LV_SYMBOL_BATTERY_FULL;
    else if (percent > 60) symbol = LV_SYMBOL_BATTERY_3;
    else if (percent > 30) symbol = LV_SYMBOL_BATTERY_2;
    else if (percent > 10) symbol = LV_SYMBOL_BATTERY_1;
    
    lv_label_set_text(ui_BatIcon, symbol);

    // Set color based on battery percentage (icon and value)
    // Reverse gradient: red at low battery
    if (percent >= 0) {
        // Invert the percentage for battery: 100% = green, 0% = red
        float inverted_pct = 100.0f - (float)percent;
        lv_color_t col = get_pct_color(inverted_pct);
        lv_obj_set_style_text_color(ui_BatIcon, col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_BatLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        // Gray color for unavailable
        lv_color_t gray = lv_color_make(128, 128, 128);
        lv_obj_set_style_text_color(ui_BatIcon, gray, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_BatLabel_Value, gray, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
