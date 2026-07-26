#ifndef UI_HARDWARE_MONITOR_H
#define UI_HARDWARE_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

// Functions
void ui_hardware_monitor_init(void);

// UI update functions with additional parameters
void ui_update_cpu(float percent, float freq_ghz);
void ui_update_gpu(float percent);
void ui_update_ram(float percent, float used_gb, float total_gb);
void ui_update_temp(float celsius, int fan_rpm);
void ui_update_network(float download_mbps, float upload_mbps);
void ui_update_battery(int percent, float power_watts);

#ifdef __cplusplus
}
#endif

#endif // UI_HARDWARE_MONITOR_H

