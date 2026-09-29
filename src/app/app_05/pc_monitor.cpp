#include "pc_monitor.h"
#include <math.h>
extern "C" {
#include "asset/ui.h"
}

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
void reading(lv_obj_t* label, float number) {
    // Three-digit readings must fit without pushing the unit outside its card.
    lv_obj_set_style_text_font(label, isfinite(number) && roundf(number) >= 100
        ? &lv_font_montserrat_14 : &ui_font_pc_temp, 0);
    value(label, number, "%.0f");
}
void meter(lv_obj_t* bar, float number) {
    int amount = isfinite(number) ? (int)roundf(number) : 0;
    lv_bar_set_value(bar, amount, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(amount >= 90 ? 0xFF7954 :
                                             amount >= 70 ? 0xF2CC60 : 0xB8EB00), LV_PART_INDICATOR);
}
void render(const pc_monitor::Frame& f) {
    text(ui_cpu_name, f.cpuName[0] ? f.cpuName : "CPU name unavailable");
    text(ui_gpu_name, f.gpuName[0] ? f.gpuName : "GPU name unavailable");
    reading(ui_cpu_temp, f.cpuTemp); reading(ui_cpu_percent, f.cpuLoad);
    reading(ui_gpu_temp, f.gpuTemp); reading(ui_gpu_percent, f.gpuLoad);
    value(ui_mhz, f.cpuMHz, "%.0f MHz");
    value(ui_gpu_ram, f.gpuMemoryMB / 1024.0f, "%.1f GB VRAM");
    meter(ui_temp2, f.cpuTemp); meter(ui_temp1, f.cpuLoad);
    meter(ui_temp3, f.gpuTemp); meter(ui_temp4, f.gpuLoad);
    const float total = f.ramUsed + f.ramAvailable;
    if (isfinite(total) && total > 0) {
        char buffer[48];
        snprintf(buffer, sizeof(buffer), "%.1f / %.1f GB", f.ramUsed, total);
        text(ui_RAM, buffer); meter(ui_Image5, 100.0f * f.ramUsed / total);
    } else { text(ui_RAM, "-- / -- GB"); meter(ui_Image5, NAN); }
}
}

namespace MOONCAKE::APPS {
PCMonitor::PCMonitor(DEVICES* device) : _device(device) { setAppInfo().name = "PCMonitor"; }
void PCMonitor::onOpen() {
    _parser.reset(); _hasData = false; _stale = false; _lastDataMs = 0;
    _previousScreen = lv_scr_act();
    ui_pc_monitor_init(); render(pc_monitor::Frame());
    ui_pc_monitor_status("Connect USB - start the PC client", false);
}
void PCMonitor::onRunning() {
    bool updated = false;
    // Bound receive work so the launcher keeps polling long-press B.
    for (size_t n = 0; n < 512 && Serial.available(); ++n)
        updated = _parser.feed((char)Serial.read(), millis()) || updated;
    if (!Serial.available()) updated = _parser.idle(millis()) || updated;
    if (updated) {
        _lastDataMs = millis(); _hasData = true; _stale = false;
        render(_parser.frame()); ui_pc_monitor_status("Live  |  Hold B to exit", true);
    } else if (_hasData && !_stale && millis() - _lastDataMs >= 5000) {
        _stale = true; render(pc_monitor::Frame());
        ui_pc_monitor_status("Data stopped - check the PC client", false);
    }
    lv_timer_handler(); delay(1);
}
void PCMonitor::onClose() {
    // Launcher normally restores the menu before calling onClose.
    if (ui_PC_Monitor && lv_scr_act() == ui_PC_Monitor && _previousScreen &&
        lv_obj_is_valid(_previousScreen)) lv_scr_load(_previousScreen);
    ui_pc_monitor_destroy(); _previousScreen = nullptr;
    _parser.reset(); _hasData = false; _stale = false;
}
}
