/* BLE keyboard transport. Static GATT resources and a shared lifecycle lease;
 * USB keyboard types remain isolated from this translation unit. */
#pragma once
#include <stdint.h>

bool bu_ble_begin();
bool bu_ble_failed();
void bu_ble_end();
bool bu_ble_connected();
void bu_ble_press(uint8_t k);
void bu_ble_release(uint8_t k);
void bu_ble_release_all();
void bu_ble_write(uint8_t c);
void bu_ble_print(const char* s);
