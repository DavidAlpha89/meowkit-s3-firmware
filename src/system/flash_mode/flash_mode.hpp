/**
 * @file flash_mode.hpp
 * @brief Serial-triggered entry into the ROM download (flash) mode.
 *
 * Developer convenience so re-flashing does not require the physical BOOT
 * button. When the trigger token arrives on the USB-CDC console, the firmware
 * sets the ESP32-S3 force-download-boot bit and resets straight into the ROM
 * serial/USB downloader — exactly the state esptool needs. A normal power
 * cycle clears the bit and boots the app again.
 */
#pragma once

#include <Arduino.h>

namespace flash_mode {

/// Trigger token; distinctive enough that ordinary console traffic never hits it.
constexpr const char* kTriggerToken = "<<MEOWKIT:BOOTLOADER>>";

/**
 * @brief Scan a serial stream for the trigger token. Call once per main loop.
 *        On a match it logs, flushes, and reboots into download mode.
 */
void poll(Stream& io);

/**
 * @brief Reboot immediately into the ROM download mode (does not return).
 */
[[noreturn]] void rebootToDownload();

} // namespace flash_mode
