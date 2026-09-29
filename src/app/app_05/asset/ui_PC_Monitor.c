// MeowKit PC Monitor: 320 x 240, native LVGL widgets, no SD assets.
#include "ui.h"
#include <string.h>
static lv_obj_t *status_label;
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int w,
                       const char *text, const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_label_set_text(obj, text); return obj;
}
static lv_obj_t *panel(int x, int y, int w, int h) {
    lv_obj_t *obj = lv_obj_create(ui_PC_Monitor);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 0, 0); lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x394437), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x19221E), 0); return obj;
}
static lv_obj_t *bar(lv_obj_t *parent, int x, int y, int w) {
    lv_obj_t *obj = lv_bar_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, 5);
    lv_bar_set_range(obj, 0, 100);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x354036), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xB8EB00), LV_PART_INDICATOR);
    lv_obj_set_style_radius(obj, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 2, LV_PART_INDICATOR);
    lv_bar_set_value(obj, 0, LV_ANIM_OFF); return obj;
}
static void readings(lv_obj_t *card, lv_obj_t **temp, lv_obj_t **load,
                     lv_obj_t **temp_bar, lv_obj_t **load_bar,
                     lv_obj_t **unit, lv_obj_t **percent) {
    label(card, 10, 32, 58, "TEMP", &lv_font_montserrat_10, 0xACB8AF);
    label(card, 81, 32, 57, "LOAD", &lv_font_montserrat_10, 0xACB8AF);
    *temp = label(card, 10, 46, 47, "--", &ui_font_pc_temp, 0xB8EB00);
    *load = label(card, 81, 46, 47, "--", &ui_font_pc_temp, 0xB8EB00);
    // Montserrat includes U+00B0; the old custom name font did not.
    *unit = label(card, 57, 62, 22, "\xC2\xB0" "C", &lv_font_montserrat_12, 0xEAF0EC);
    *percent = label(card, 130, 62, 17, "%", &lv_font_montserrat_12, 0xEAF0EC);
    *temp_bar = bar(card, 10, 90, 62); *load_bar = bar(card, 81, 90, 62);
}
void ui_PC_Monitor_screen_init(void) {
    ui_PC_Monitor = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PC_Monitor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PC_Monitor, lv_color_hex(0x0D1411), 0);
    lv_obj_set_style_pad_all(ui_PC_Monitor, 0, 0);
    label(ui_PC_Monitor, 10, 5, 300, "PC MONITOR", &lv_font_montserrat_14, 0xB8EB00);
    lv_obj_t *cpu = panel(6, 26, 151, 136), *gpu = panel(163, 26, 151, 136);
    ui_cpu_name = label(cpu, 10, 8, 131, "CPU", &lv_font_montserrat_12, 0xEAF0EC);
    ui_gpu_name = label(gpu, 10, 8, 131, "GPU", &lv_font_montserrat_12, 0xEAF0EC);
    lv_label_set_long_mode(ui_cpu_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(ui_gpu_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(ui_cpu_name, 20, 0);
    lv_obj_set_style_anim_speed(ui_gpu_name, 20, 0);
    readings(cpu, &ui_cpu_temp, &ui_cpu_percent, &ui_temp2, &ui_temp1, &ui_symbol_1, &ui_symbol_2);
    readings(gpu, &ui_gpu_temp, &ui_gpu_percent, &ui_temp3, &ui_temp4, &ui_symbol_3, &ui_symbol_4);
    ui_mhz = label(cpu, 10, 111, 131, "-- MHz", &lv_font_montserrat_12, 0xACB8AF);
    ui_gpu_ram = label(gpu, 10, 111, 131, "-- GB VRAM", &lv_font_montserrat_12, 0xACB8AF);
    lv_obj_t *ram = panel(6, 168, 308, 46);
    label(ram, 10, 6, 72, "RAM USED", &lv_font_montserrat_10, 0xACB8AF);
    ui_RAM = label(ram, 90, 5, 207, "-- / -- GB", &lv_font_montserrat_14, 0xEAF0EC);
    lv_obj_set_style_text_align(ui_RAM, LV_TEXT_ALIGN_RIGHT, 0);
    ui_Image5 = bar(ram, 10, 31, 286);
    status_label = label(ui_PC_Monitor, 10, 221, 300, "Connect USB - start the PC client",
                         &lv_font_montserrat_10, 0xACB8AF);
}
void ui_pc_monitor_status(const char *text, bool live) {
    if (!status_label) return;
    if (strcmp(lv_label_get_text(status_label), text)) lv_label_set_text(status_label, text);
    lv_obj_set_style_text_color(status_label, lv_color_hex(live ? 0xB8EB00 : 0xACB8AF), 0);
}
void ui_pc_monitor_destroy(void) {
    if (!ui_PC_Monitor || lv_scr_act() == ui_PC_Monitor) return;
    lv_obj_del(ui_PC_Monitor); ui_PC_Monitor = NULL; status_label = NULL;
    ui_pc_monitor_bg = ui_temp1 = ui_temp2 = ui_cpu_name = NULL;
    ui_cpu_temp = ui_cpu_percent = ui_symbol_1 = ui_symbol_2 = NULL;
    ui_gpu_name = ui_temp3 = ui_temp4 = ui_gpu_temp = ui_gpu_percent = NULL;
    ui_symbol_3 = ui_symbol_4 = ui_Image5 = ui_RAM = ui_gpu_ram = ui_mhz = NULL;
}
