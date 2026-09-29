#pragma once
// Bounded legacy HardwareSerialMonitor parser; also compiled by host tests.
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace pc_monitor {
struct Frame {
    float cpuTemp = NAN, cpuLoad = NAN, gpuTemp = NAN, gpuLoad = NAN;
    float ramUsed = NAN, ramAvailable = NAN, cpuMHz = NAN, gpuMemoryMB = NAN;
    float gpuMHz = NAN, gpuFanLoad = NAN, gpuFanRPM = NAN;
    char cpuName[96] = {}, gpuName[96] = {};
};
class Parser {
public:
    void reset() { *this = Parser(); }
    const Frame& frame() const { return published_; }
    bool feed(char ch, uint32_t now) {
        lastByte_ = now;
        if (ch == '\r') return false;
        if (ch == '|' || ch == '\n') {
            bool ready = false;
            if (overflow_) { active_ = false; seen_ = 0; }
            else if (length_) { token_[length_] = 0; ready = consume(); }
            length_ = 0; overflow_ = false;
            if (ch == '\n') ready = publish() || ready;
            return ready;
        }
        if ((unsigned char)ch < 32) return false;
        if (length_ + 1 < sizeof(token_)) token_[length_++] = ch;
        else overflow_ = true;
        return false;
    }
    bool idle(uint32_t now) {
        // Accept legacy senders that end with a pipe but omit the newline.
        if ((uint32_t)(now - lastByte_) >= 150 && !length_ && !overflow_)
            return publish();
        if ((uint32_t)(now - lastByte_) >= 1500) {
            length_ = 0; overflow_ = false; active_ = false; seen_ = 0;
        }
        return false;
    }
private:
    char token_[256] = {};
    size_t length_ = 0;
    bool overflow_ = false, active_ = false;
    unsigned seen_ = 0;
    uint32_t lastByte_ = 0;
    Frame pending_, published_;
    static void name(char* dst, const char* first, size_t size) {
        while (size && *first == ' ') { ++first; --size; }
        while (size && first[size - 1] == ' ') --size;
        if (size > 95) size = 95;
        memcpy(dst, first, size); dst[size] = 0;
    }
    static float number(const char* s, float max, const char* unit = "") {
        // Older clients use the Windows decimal separator (e.g. 13,8GB).
        char normalized[64];
        size_t size = strlen(s);
        if (size >= sizeof(normalized)) return NAN;
        memcpy(normalized, s, size + 1);
        for (size_t i = 0; i < size; ++i) if (normalized[i] == ',') normalized[i] = '.';
        s = normalized;
        char* end;
        float value = strtof(s, &end);
        if (end == s || !isfinite(value) || value < 0 || value > max) return NAN;
        while (*end == ' ') ++end;
        if (*end && strcmp(end, unit)) return NAN;
        return value;
    }
    static bool temperatureLoad(char* s, float& temperature, float& load) {
        char* split = strchr(s, 'c');
        if (!split) return false;
        *split = 0;
        temperature = number(s, 150);
        load = number(split + 1, 100, "%");
        return true;
    }
    bool publish() {
        bool ready = active_ && seen_ == 15;
        if (ready) published_ = pending_;
        active_ = false; seen_ = 0;
        return ready;
    }
    bool consume() {
        if (token_[0] == 'C' && ((token_[1] >= '0' && token_[1] <= '9') ||
            token_[1] == 'c' || token_[1] == '-' || token_[1] == '+' || token_[1] == 'n')) {
            bool ready = publish();
            pending_ = Frame();
            active_ = temperatureLoad(token_ + 1, pending_.cpuTemp, pending_.cpuLoad);
            seen_ = active_ ? 1 : 0;
            return ready;
        }
        if (!active_) return false;
        if (!strncmp(token_, "GPU:", 4)) {
            name(pending_.gpuName, token_ + 4, strlen(token_ + 4));
        } else if (!strncmp(token_, "CPU:", 4)) {
            char* gpu = strstr(token_ + 4, "GPU:");
            name(pending_.cpuName, token_ + 4, gpu ? size_t(gpu - token_ - 4) : strlen(token_ + 4));
            if (gpu) name(pending_.gpuName, gpu + 4, strlen(gpu + 4));
        } else if (!strncmp(token_, "GMT", 3)) {
            pending_.gpuMemoryMB = number(token_ + 3, 1048576, "MB");
        } else if (!strncmp(token_, "CHC", 3)) {
            pending_.cpuMHz = number(token_ + 3, 20000, "MHz");
        } else if (!strncmp(token_, "GCC", 3)) {
            pending_.gpuMHz = number(token_ + 3, 20000, "MHz");
        } else if (!strncmp(token_, "GFANL", 5)) {
            pending_.gpuFanLoad = number(token_ + 5, 100, "%");
        } else if (!strncmp(token_, "GRPM", 4)) {
            pending_.gpuFanRPM = number(token_ + 4, 100000, "RPM");
        } else if (token_[0] == 'G' && token_[1] != 'M') {
            if (temperatureLoad(token_ + 1, pending_.gpuTemp, pending_.gpuLoad)) seen_ |= 2;
        } else if (!strncmp(token_, "RA", 2)) {
            pending_.ramAvailable = number(token_ + 2, 1048576, "GB"); seen_ |= 8;
        } else if (token_[0] == 'R' && token_[1] != 'L') {
            pending_.ramUsed = number(token_ + 1, 1048576, "GB"); seen_ |= 4;
        }
        return false;
    }
};
}
