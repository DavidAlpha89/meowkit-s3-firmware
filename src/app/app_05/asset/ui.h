#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
LV_FONT_DECLARE(ui_font_pc_temp);
LV_IMG_DECLARE(pc_monitor_background);
LV_IMG_DECLARE(pc_monitor_overlay);
extern lv_obj_t *ui_PC_Monitor;
extern lv_obj_t *ui_cpu_name, *ui_gpu_name, *ui_gpu_brand;
extern lv_obj_t *ui_cpu_temp, *ui_cpu_percent, *ui_gpu_temp, *ui_gpu_percent;
extern lv_obj_t *ui_cpu_mhz, *ui_gpu_mhz, *ui_gpu_ram, *ui_ram_total, *ui_ram_percent;
extern lv_obj_t *ui_gpu_fan_load, *ui_gpu_fan_rpm;
extern lv_obj_t *ui_cpu_temp_bar, *ui_cpu_load_bar, *ui_gpu_temp_bar, *ui_gpu_load_bar, *ui_ram_bar;
void ui_pc_monitor_init(void);
void ui_pc_monitor_status(const char *text, bool live);
void ui_pc_monitor_meter(lv_obj_t *meter, int value);
void ui_pc_monitor_destroy(void);
#ifdef __cplusplus
}
#endif
