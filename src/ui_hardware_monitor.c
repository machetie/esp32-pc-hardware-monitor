#include "ui_hardware_monitor.h"
#include "ui.h"
#include <stdio.h>

// Helper: return a color on a green→yellow→red gradient based on a 0–100% value
// lv_color_mix(c1, c2, ratio): ratio=255 gives c1, ratio=0 gives c2
static lv_color_t get_pct_color(float pct) {
    // Clamp to valid range
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    if (pct < 60.0f) {
        // Green→Yellow (0–60%)
        // At pct=0: ratio=0 → green, At pct=60: ratio=255 → yellow
        uint8_t ratio = (uint8_t)(pct / 60.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 255, 0), lv_color_make(0, 255, 0), ratio);
    } else {
        // Yellow→Red (60–100%)
        // At pct=60: ratio=0 → yellow, At pct=100: ratio=255 → red
        uint8_t ratio = (uint8_t)((pct - 60.0f) / 40.0f * 255.0f);
        return lv_color_mix(lv_color_make(255, 0, 0), lv_color_make(255, 255, 0), ratio);
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

lv_obj_t * ui_BatLabel_Prefix;
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

    // Line 1: CPU Information (Y=2)
    ui_CPULabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_CPULabel_Prefix, 10);
    lv_obj_set_y(ui_CPULabel_Prefix, 2);
    lv_label_set_text(ui_CPULabel_Prefix, "CPU:");
    lv_obj_set_style_text_color(ui_CPULabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_CPULabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CPULabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_CPULabel_Value, 60);
    lv_obj_set_y(ui_CPULabel_Value, 2);
    lv_label_set_text(ui_CPULabel_Value, "0.0%");
    lv_obj_set_style_text_color(ui_CPULabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_CPULabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 2: GPU Information (Y=30)
    ui_GPULabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_GPULabel_Prefix, 10);
    lv_obj_set_y(ui_GPULabel_Prefix, 30);
    lv_label_set_text(ui_GPULabel_Prefix, "GPU:");
    lv_obj_set_style_text_color(ui_GPULabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_GPULabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_GPULabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_GPULabel_Value, 60);
    lv_obj_set_y(ui_GPULabel_Value, 30);
    lv_label_set_text(ui_GPULabel_Value, "0.0%");
    lv_obj_set_style_text_color(ui_GPULabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_GPULabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 3: RAM Information (Y=58)
    ui_RAMLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_RAMLabel_Prefix, 10);
    lv_obj_set_y(ui_RAMLabel_Prefix, 58);
    lv_label_set_text(ui_RAMLabel_Prefix, "RAM:");
    lv_obj_set_style_text_color(ui_RAMLabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_RAMLabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_RAMLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_RAMLabel_Value, 60);
    lv_obj_set_y(ui_RAMLabel_Value, 58);
    lv_label_set_text(ui_RAMLabel_Value, "0.0%");
    lv_obj_set_style_text_color(ui_RAMLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_RAMLabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 4: Temperature Information (Y=86)
    ui_TempLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_TempLabel_Prefix, 10);
    lv_obj_set_y(ui_TempLabel_Prefix, 86);
    lv_label_set_text(ui_TempLabel_Prefix, "TEMP:");
    lv_obj_set_style_text_color(ui_TempLabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempLabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TempLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_TempLabel_Value, 75);
    lv_obj_set_y(ui_TempLabel_Value, 86);
    lv_label_set_text(ui_TempLabel_Value, "0.0°C");
    lv_obj_set_style_text_color(ui_TempLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempLabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 5: Network Information (Y=114)
    ui_NetLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_NetLabel_Prefix, 10);
    lv_obj_set_y(ui_NetLabel_Prefix, 114);
    lv_label_set_text(ui_NetLabel_Prefix, "NET:");
    lv_obj_set_style_text_color(ui_NetLabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NetLabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NetLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_NetLabel_Value, 60);
    lv_obj_set_y(ui_NetLabel_Value, 114);
    lv_label_set_text(ui_NetLabel_Value, "(not available)");
    lv_obj_set_style_text_color(ui_NetLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NetLabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Line 6: Battery Information (Y=142)
    ui_BatLabel_Prefix = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_BatLabel_Prefix, 10);
    lv_obj_set_y(ui_BatLabel_Prefix, 142);
    lv_label_set_text(ui_BatLabel_Prefix, "BAT:");
    lv_obj_set_style_text_color(ui_BatLabel_Prefix, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BatLabel_Prefix, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BatLabel_Value = lv_label_create(ui_HWMonScreen);
    lv_obj_set_x(ui_BatLabel_Value, 60);
    lv_obj_set_y(ui_BatLabel_Value, 142);
    lv_label_set_text(ui_BatLabel_Value, "(not available)");
    lv_obj_set_style_text_color(ui_BatLabel_Value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BatLabel_Value, &lv_font_vt323_32, LV_PART_MAIN | LV_STATE_DEFAULT);

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
        snprintf(text, sizeof(text), "%.1f%% %.1f/%.1fGB", percent, used_gb, total_gb);
    } else {
        snprintf(text, sizeof(text), "%.1f%%", percent);
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
        snprintf(text, sizeof(text), "%.1f°C %drpm", celsius, fan_rpm);
    } else {
        snprintf(text, sizeof(text), "%.1f°C", celsius);
    }
    lv_label_set_text(ui_TempLabel_Value, text);

    // Color: blue (<50°C), green (50–70°C), yellow (70–85°C), red (≥85°C)
    // Apply to value label only
    lv_color_t col;
    if (celsius >= 85.0f) {
        col = lv_color_make(255, 0, 0);  // Red - Hot! (R=255, G=0, B=0)
    } else if (celsius >= 70.0f) {
        col = lv_color_make(255, 255, 0);  // Yellow - Warm (R=255, G=255, B=0)
    } else if (celsius >= 50.0f) {
        col = lv_color_make(0, 255, 0);  // Green - Normal (R=0, G=255, B=0)
    } else {
        col = lv_color_make(0, 174, 239);  // Blue - Cool (R=0, G=174, B=239)
    }
    lv_obj_set_style_text_color(ui_TempLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_update_network(float download_mbps, float upload_mbps) {
    char text[64];
    
    // Convert MB/s to KB/s
    float download_kbps = download_mbps * 1024.0f;
    float upload_kbps = upload_mbps * 1024.0f;
    
    // Dynamic display: show KB/s for speeds <1 MB/s, otherwise show MB/s
    if (download_mbps < 1.0f && upload_mbps < 1.0f) {
        // Both speeds low - show in KB/s
        snprintf(text, sizeof(text), "D%.0f U%.0f KB/s", download_kbps, upload_kbps);
    } else if (download_mbps < 1.0f) {
        // Download low, upload high
        snprintf(text, sizeof(text), "D%.0fKB U%.1fMB/s", download_kbps, upload_mbps);
    } else if (upload_mbps < 1.0f) {
        // Download high, upload low
        snprintf(text, sizeof(text), "D%.1fMB U%.0fKB/s", download_mbps, upload_kbps);
    } else {
        // Both high - show in MB/s
        snprintf(text, sizeof(text), "D%.1f U%.1f MB/s", download_mbps, upload_mbps);
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
    char text[48];
    // Display battery percentage and power - value only
    if (percent >= 0 && power_watts >= 0.0) {
        snprintf(text, sizeof(text), "%d%% %.1fW", percent, power_watts);
    } else {
        snprintf(text, sizeof(text), "(not available)");
    }
    lv_label_set_text(ui_BatLabel_Value, text);

    // Set color based on battery percentage (value label only)
    // Reverse gradient: red at low battery
    if (percent >= 0) {
        // Invert the percentage for battery: 100% = green, 0% = red
        float inverted_pct = 100.0f - (float)percent;
        lv_color_t col = get_pct_color(inverted_pct);
        lv_obj_set_style_text_color(ui_BatLabel_Value, col, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        // Gray color for unavailable
        lv_obj_set_style_text_color(ui_BatLabel_Value, lv_color_make(128, 128, 128), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
