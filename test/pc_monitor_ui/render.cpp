#include "../../src/app/app_05/asset/ui.h"
#include "../../src/app/app_05/pc_monitor_view.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t ticks;
extern "C" uint32_t millis(void) { return ticks; }
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
        pc_monitor::Parser parser;
        const char* sample = "C32c50%|G79c99%|R8|RA8|CHC4800|GMT32768|GCC2200|GFANL62|GRPM944|CPU:INTEL I9-14900K|GPU:NVIDIA RTX 5090|\n";
        for (const char* c=sample; *c; ++c) parser.feed(*c, ticks);
        pc_monitor::Frame data = parser.frame();
        pc_monitor::render(data); ui_pc_monitor_status("Live",true);
        lv_obj_update_layout(ui_PC_Monitor);
        fits(ui_cpu_temp); fits(ui_cpu_percent); fits(ui_ram_total); fits(ui_cpu_mhz); fits(ui_gpu_ram);
        fits(ui_gpu_mhz); fits(ui_gpu_fan_rpm); fits(ui_ram_percent);
        assert(lv_obj_get_x(ui_gpu_mhz) + lv_obj_get_width(ui_gpu_mhz) == 102);
        const uint32_t palette[] = {0x22D3EE,0x4D9FFF,0x9B8AFB,0xFFBE55,0xFF706B};
        for (int segment=0; segment<5; ++segment)
            assert(lv_obj_get_style_bg_color(lv_obj_get_child(ui_cpu_load_bar,segment),0).full == lv_color_hex(palette[segment]).full);
        assert(pc_monitor_background.header.w == 320 && pc_monitor_background.header.h == 240);
        assert(pc_monitor_overlay.header.w == 302 && pc_monitor_overlay.header.h == 230);
        assert(lv_obj_get_style_width(lv_obj_get_child(ui_ram_bar,0),0) == 40);
        assert(lv_obj_get_child_cnt(ui_ram_bar) == 5);
        for (int segment=0; segment<5; ++segment) {
            lv_obj_t *fill = lv_obj_get_child(ui_ram_bar,segment);
            assert(lv_obj_get_style_bg_color(fill,0).full == lv_color_hex(palette[segment]).full);
            if (segment < 3) {
                assert(lv_obj_get_style_height(fill,0) == (segment == 2 ? 5 : 10));
                assert(lv_obj_get_y(fill) == (segment == 2 ? 25 : 40 - segment * 10));
            } else assert(lv_obj_has_flag(fill,LV_OBJ_FLAG_HIDDEN));
        }
        if(i==0) screenshot("output/pc-monitor-live.bmp");
        data.cpuTemp=150; data.cpuLoad=data.gpuLoad=100; data.cpuMHz=data.gpuMHz=20000;
        data.ramUsed=128; data.ramAvailable=0; data.gpuFanRPM=100000;
        pc_monitor::render(data); lv_obj_update_layout(ui_PC_Monitor);
        fits(ui_cpu_temp); fits(ui_cpu_percent); fits(ui_cpu_mhz); fits(ui_gpu_mhz); fits(ui_ram_total); fits(ui_ram_percent); fits(ui_gpu_fan_rpm);
        if(i==0) screenshot("output/pc-monitor-100.bmp");
        for (int segment=0; segment<5; ++segment) {
            lv_obj_t *fill = lv_obj_get_child(ui_ram_bar,segment);
            assert(!lv_obj_has_flag(fill,LV_OBJ_FLAG_HIDDEN));
            assert(lv_obj_get_style_height(fill,0) == 10);
            assert(lv_obj_get_y(fill) == 40 - segment * 10);
        }
        pc_monitor::render(pc_monitor::Frame());
        assert(!strcmp(lv_label_get_text(ui_gpu_fan_rpm),"--"));
        assert(lv_obj_has_flag(lv_obj_get_child(ui_ram_bar,0),LV_OBJ_FLAG_HIDDEN));
        for (int segment=0; segment<5; ++segment)
            assert(lv_obj_has_flag(lv_obj_get_child(ui_ram_bar,segment),LV_OBJ_FLAG_HIDDEN));
        ui_pc_monitor_status("Data stopped - check the PC client",false);
        if(i==0) screenshot("output/pc-monitor-stale.bmp");
        if(i==0) {
            lv_color_t partial[320 * 240]; memcpy(partial,frame,sizeof(frame));
            lv_obj_invalidate(ui_PC_Monitor); screenshot("output/pc-monitor-stale-full.bmp");
            assert(!memcmp(partial,frame,sizeof(frame))); // No stale pixels after partial redraws.
        }
        lv_scr_load(previous); ui_pc_monitor_destroy(); assert(ui_PC_Monitor==NULL);
    }
    puts("Flash artwork, protocol-to-view rendering, numeric bounds, missing data and 30 open/close cycles passed");
}
