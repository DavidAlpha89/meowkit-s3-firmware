/**
 * @file flash_mode.cpp
 * @brief Serial-triggered entry into ROM download mode — implementation.
 */
#include "flash_mode.hpp"

#include <cstring>
#include <esp_system.h>
#include <soc/rtc_cntl_reg.h>

namespace {

// Rolling buffer holding the tail of recently received console bytes, so the
// token is matched even when it arrives split across reads.
char   s_buf[48];
size_t s_len = 0;

} // namespace

namespace flash_mode {

void rebootToDownload()
{
    // Force the ROM to enter serial/USB download mode on the next reset,
    // then reset. The bit lives in the RTC domain and is cleared by a power
    // cycle, so after flashing a single power cycle returns to the app.
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
    while (true) { /* unreachable: esp_restart() does not return */ }
}

void poll(Stream& io)
{
    const size_t token_len = strlen(kTriggerToken);

    while (io.available() > 0) {
        const char c = static_cast<char>(io.read());

        if (s_len >= sizeof(s_buf) - 1) {
            // Drop the oldest byte to make room (keep the most recent tail).
            memmove(s_buf, s_buf + 1, sizeof(s_buf) - 2);
            s_len = sizeof(s_buf) - 2;
        }
        s_buf[s_len++] = c;
        s_buf[s_len]   = '\0';

        if (s_len >= token_len &&
            strcmp(s_buf + (s_len - token_len), kTriggerToken) == 0) {
            io.printf("[FLASH] token received — rebooting into download mode\n");
            io.flush();
            delay(50);
            rebootToDownload();
        }
    }
}

} // namespace flash_mode
