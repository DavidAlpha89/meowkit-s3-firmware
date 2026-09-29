#include "../src/app/app_05/pc_monitor_protocol.h"
#include <cassert>
#include <string>
#include <iostream>
using pc_monitor::Parser;
static bool send(Parser& p, const std::string& s, uint32_t now = 0) {
    bool ready = false;
    for (char c : s) ready = p.feed(c, now) || ready;
    return ready;
}
static const char* sample = "C36c13%|G44c46%|R13.8GB|RA2.2GB|CHC1600|GMT12288|CPU:Intel Core i7-7700KGPU:NVIDIA GeForce RTX 4070 SUPER|";
int main() {
    Parser p;
    assert(!send(p, sample)); assert(!p.idle(149)); assert(p.idle(150));
    assert(p.frame().cpuTemp == 36 && p.frame().gpuLoad == 46);
    assert(fabs(p.frame().ramUsed - 13.8f) < .001);
    assert(p.frame().cpuMHz == 1600 && p.frame().gpuMemoryMB == 12288);
    assert(std::string(p.frame().cpuName) == "Intel Core i7-7700K");
    assert(std::string(p.frame().gpuName) == "NVIDIA GeForce RTX 4070 SUPER");
    assert(!p.idle(200));
    p.reset(); assert(!send(p, "C50c20%|G40c30%|")); assert(!p.idle(200));
    assert(send(p, std::string(sample) + "\r\n", 300));
    p.reset(); assert(!send(p, "C36c")); assert(!p.idle(200));
    assert(!send(p, "13%|G44c46%|R13.8GB|RA2.2GB|", 201)); assert(p.idle(351));
    p.reset(); assert(!send(p, std::string(1000, 'X') + "|"));
    assert(send(p, std::string(sample) + "\n"));
    p.reset(); assert(send(p, std::string(sample) + sample)); assert(p.idle(150));
    p.reset(); assert(send(p, "Cnanc101%|G-1c50%|Rnan|RA2|\n"));
    assert(isnan(p.frame().cpuTemp) && isnan(p.frame().cpuLoad));
    assert(isnan(p.frame().gpuTemp) && isnan(p.frame().ramUsed));
    assert(p.frame().gpuLoad == 50);
    p.reset(); assert(send(p, "C100c100%|G100c0%|R0|RA16|CPU:AMD Ryzen|GPU:AMD Radeon|\n"));
    assert(p.frame().cpuTemp == 100 && p.frame().cpuLoad == 100);
    assert(std::string(p.frame().cpuName) == "AMD Ryzen");
    p.reset(); assert(send(p, "C36c 13%|G44c 46%|R13,8GB|RA2,2|RL86|GMT12288|GMU1000|GML8|GFANL0|GRPM0|GPWR30|CPU:Intel Core i7-7700KGPU:NVIDIA GeForce RTX 4070 SUPER|GCC2500||GMC10000||GSC0||CHC1600|\r\n"));
    assert(fabs(p.frame().ramUsed - 13.8f) < .001);
    assert(p.frame().cpuMHz == 1600 && p.frame().gpuTemp == 44);
    assert(p.frame().gpuMHz == 2500 && p.frame().gpuFanRPM == 0 && p.frame().gpuFanLoad == 0);
    p.reset(); assert(send(p, "C32c50%|G79c99%|R8|RA8|GCC2200MHz|GFANL62%|GRPM944RPM|\n"));
    assert(p.frame().gpuMHz == 2200 && p.frame().gpuFanRPM == 944 && p.frame().gpuFanLoad == 62);
    assert(send(p, "C32c50%|G79c99%|R8|RA8|GCCnan|GFANL101|GRPM-1|\n"));
    assert(isnan(p.frame().gpuMHz) && isnan(p.frame().gpuFanRPM) && isnan(p.frame().gpuFanLoad));
    assert(send(p, std::string(sample) + "\n"));
    assert(isnan(p.frame().gpuMHz) && isnan(p.frame().gpuFanRPM) && isnan(p.frame().gpuFanLoad));
    // Unsigned millis arithmetic remains valid across wrap-around.
    p.reset(); assert(!send(p, sample, UINT32_MAX - 50)); assert(p.idle(100));
    p.reset(); send(p, "truncated"); assert(!p.idle(1600));
    assert(send(p, std::string(sample) + "\n", 1700));
    std::cout << "PC Monitor protocol regression tests passed\n";
}
