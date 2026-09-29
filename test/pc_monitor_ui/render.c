#include "../../src/app/app_05/asset/ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t ticks;
uint32_t millis(void) { return ticks; }
static lv_color_t frame[320 * 240], buffer[320 * 20];
static void flush(lv_disp_drv_t *drv, const lv_area_t *a, lv_color_t *p) {
    for (int y = a->y1; y <= a->y2; ++y)
        for (int x = a->x1; x <= a->x2; ++x) {
            if (x >= 0 && x < 320 && y >= 0 && y < 240) frame[y * 320 + x] = *p;
            ++p;
        }
    lv_disp_flush_ready(drv);
}
static void screenshot(const char *path) {
    ticks += 40; lv_timer_handler(); lv_refr_now(NULL);
    // Uncompressed 24-bit BMP. Width 320 has no row padding.
    unsigned char header[54] = {'B','M'};
    uint32_t size = 54 + 320 * 240 * 3, offset = 54, dib = 40, w = 320, h = 240;
    memcpy(header+2,&size,4); memcpy(header+10,&offset,4); memcpy(header+14,&dib,4);
    memcpy(header+18,&w,4); memcpy(header+22,&h,4); header[26]=1; header[28]=24;
    FILE *f = fopen(path,"wb"); assert(f); fwrite(header,1,54,f);
    for (int y=239;y>=0;--y) for(int x=0;x<320;++x) {
        lv_color32_t c; c.full = lv_color_to32(frame[y*320+x]);
        unsigned char bgr[3]={c.ch.blue,c.ch.green,c.ch.red}; fwrite(bgr,1,3,f);
    }
    fclose(f);
}
static void fits(lv_obj_t *label) {
    lv_point_t size;
    lv_txt_get_size(&size, lv_label_get_text(label), lv_obj_get_style_text_font(label,0),
                   0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    assert(size.x <= lv_obj_get_width(label));
}
int main(void) {
    lv_init();
    lv_disp_draw_buf_t draw; lv_disp_draw_buf_init(&draw,buffer,NULL,320*20);
    lv_disp_drv_t drv; lv_disp_drv_init(&drv); drv.hor_res=320; drv.ver_res=240;
    drv.flush_cb=flush; drv.draw_buf=&draw; lv_disp_drv_register(&drv);
    lv_obj_t *previous=lv_scr_act();
    for(int i=0;i<30;++i) {
        ui_pc_monitor_init();
        lv_label_set_text(ui_cpu_name,"Intel Core i7-7700K");
        lv_label_set_text(ui_gpu_name,"NVIDIA GeForce RTX 4070 SUPER");
        lv_label_set_text(ui_cpu_temp,"36"); lv_label_set_text(ui_cpu_percent,"13");
        lv_label_set_text(ui_gpu_temp,"44"); lv_label_set_text(ui_gpu_percent,"46");
        lv_label_set_text(ui_RAM,"13.8 / 16.0 GB"); lv_label_set_text(ui_mhz,"1600 MHz");
        lv_label_set_text(ui_gpu_ram,"12.0 GB VRAM");
        lv_bar_set_value(ui_temp1,13,LV_ANIM_OFF); lv_bar_set_value(ui_temp2,36,LV_ANIM_OFF);
        lv_bar_set_value(ui_temp3,44,LV_ANIM_OFF); lv_bar_set_value(ui_temp4,46,LV_ANIM_OFF);
        lv_bar_set_value(ui_Image5,86,LV_ANIM_OFF); ui_pc_monitor_status("Live  |  Hold B to exit",true);
        lv_obj_update_layout(ui_PC_Monitor);
        fits(ui_cpu_temp); fits(ui_cpu_percent); fits(ui_RAM); fits(ui_mhz); fits(ui_gpu_ram); fits(ui_symbol_1);
        lv_font_glyph_dsc_t glyph; assert(lv_font_get_glyph_dsc(&lv_font_montserrat_12,&glyph,0xB0,'C'));
        if(i==0) screenshot("output/pc-monitor-live.bmp");
        lv_obj_set_style_text_font(ui_cpu_percent,&lv_font_montserrat_14,0);
        lv_label_set_text(ui_cpu_percent,"100"); lv_obj_update_layout(ui_PC_Monitor); fits(ui_cpu_percent);
        if(i==0) screenshot("output/pc-monitor-100.bmp");
        lv_scr_load(previous); ui_pc_monitor_destroy(); assert(ui_PC_Monitor==NULL);
    }
    puts("LVGL layout, degree glyph and 30 open/close cycles passed");
}
