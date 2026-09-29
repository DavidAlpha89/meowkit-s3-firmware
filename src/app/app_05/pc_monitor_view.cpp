#include "pc_monitor_view.h"
#include "asset/ui.h"
#include <stdio.h>

namespace {
void text(lv_obj_t* label, const char* content) {
    if (strcmp(lv_label_get_text(label), content)) lv_label_set_text(label, content);
}
void value(lv_obj_t* label, float number, const char* format) {
    char buffer[40];
    if (isfinite(number)) snprintf(buffer, sizeof(buffer), format, number);
    else snprintf(buffer, sizeof(buffer), "--");
    text(label, buffer);
}
void digits(lv_obj_t* label, float number) {
    char buffer[24];
    if (isfinite(number)) snprintf(buffer, sizeof(buffer), "%.0f", number);
    else snprintf(buffer, sizeof(buffer), "--");
    lv_point_t size;
    lv_txt_get_size(&size, buffer, &ui_font_pc_temp, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const lv_font_t* font = size.x <= lv_obj_get_style_width(label, 0)
                         ? &ui_font_pc_temp : &lv_font_montserrat_14;
    if (lv_obj_get_style_text_font(label, 0) != font) lv_obj_set_style_text_font(label, font, 0);
    text(label, buffer);
}
void meter(lv_obj_t* obj, float number) {
    ui_pc_monitor_meter(obj, isfinite(number) ? (int)roundf(number) : 0);
}
}
namespace pc_monitor {
void render(const Frame& f) {
    text(ui_cpu_name, f.cpuName[0] ? f.cpuName : "CPU");
    // Separate brand/model in the narrow artwork pane; long models scroll.
    const char* gpu = f.gpuName;
    const char* model = strstr(gpu, "RTX ");
    if (!model) model = strstr(gpu, "GTX ");
    if (!model) model = strstr(gpu, "RX ");
    if (!model) model = strstr(gpu, "Arc ");
    if (model) {
        const char* brand = strstr(gpu, "NVIDIA") ? "NVIDIA" : strstr(gpu, "AMD") ? "AMD" : strstr(gpu, "Intel") ? "Intel" : "GPU";
        text(ui_gpu_brand, brand); text(ui_gpu_name, model);
    } else { text(ui_gpu_brand, ""); text(ui_gpu_name, *gpu ? gpu : "GPU"); }
    digits(ui_cpu_temp, f.cpuTemp); digits(ui_cpu_percent, f.cpuLoad);
    digits(ui_gpu_temp, f.gpuTemp); digits(ui_gpu_percent, f.gpuLoad);
    digits(ui_cpu_mhz, f.cpuMHz); digits(ui_gpu_mhz, f.gpuMHz);
    value(ui_gpu_ram, f.gpuMemoryMB / 1024.0f, "%.3g GB");
    value(ui_gpu_fan_load, f.gpuFanLoad, "%.0f%%");
    value(ui_gpu_fan_rpm, f.gpuFanRPM, "%.0f RPM");
    meter(ui_cpu_temp_bar, f.cpuTemp); meter(ui_cpu_load_bar, f.cpuLoad);
    meter(ui_gpu_temp_bar, f.gpuTemp); meter(ui_gpu_load_bar, f.gpuLoad);
    const float total = f.ramUsed + f.ramAvailable;
    const float used = isfinite(total) && total > 0 ? 100.0f * f.ramUsed / total : NAN;
    digits(ui_ram_total, isfinite(total) && total > 0 ? total : NAN);
    digits(ui_ram_percent, used); meter(ui_ram_bar, used);
}
}
