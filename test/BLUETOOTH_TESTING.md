# Bluetooth integration validation

> Current exception: app06 Air Mouse has been rolled back to its pre-manager
> implementation at the user's request. It starts/stops Bluetooth independently
> of the Settings switch. Manager-specific Air Mouse expectations below describe
> the superseded integration and are not release acceptance criteria for app06.
> Reconfirm pairing, clicks, scrolling, motion and exit on hardware.

## Behavior

- Enable Bluetooth in Settings before opening a Bluetooth application.
  This setting permits access; it does not advertise a separate system device.
- Only one application can own the radio. Another request is rejected rather
  than tearing down the current owner.
- Air Mouse retains its stable app-specific random address and name.
  BadUSB retains its public address and name. Host GATT caches may require
  removing the old pairing once after upgrading.
- BadUSB becomes ready only after encryption and notification subscription.
  A disconnect or failed send aborts execution. Reopen the script and press A
  to run from the beginning; there is no automatic replay.
- Master-off and application exit stop the session. Controller memory is not
  permanently released, so another application can initialize Bluetooth.
- Shutdown waits at most 1500 ms for stop/disconnect events before stack teardown.
  On failure the manager denies new sessions. Toggle Bluetooth off/on to retry
  only after the stack has fully stopped; restart the device if it cannot stop.
- PC Monitor remains USB CDC. app07 changes are lifecycle guards only.

## Host regression tests

Run from the project root with MinGW g++ and PlatformIO installed.
The output directory is ignored by Git.

```powershell
g++ -std=c++17 -Wall -Wextra test/bluetooth_manager_test.cpp -o output/bluetooth_manager_test.exe
./output/bluetooth_manager_test.exe
g++ -std=c++17 -Wall -Wextra test/air_mouse_hid_test.cpp -o output/air_mouse_hid_test.exe
./output/air_mouse_hid_test.exe
g++ -std=c++17 -Wall -Wextra -DARDUINO_ARCH_ESP32 -I"$env:USERPROFILE/.platformio/packages/framework-arduinoespressif32/libraries/BLE/src" test/badusb_ble_test.cpp -o output/badusb_ble_test.exe
./output/badusb_ble_test.exe
pio run -e esp32s3box
```

The tests use simulated controller events. They do not validate RF behavior,
Windows driver compatibility, actual bonding, battery life or heap stability.

## Hardware release gate - pending

### Air Mouse connection failure trace

For the Windows "try connecting your device again" regression, capture serial
output at 115200 baud from app entry through the failed connection. Keep lines
starting with [BT] and [AirMouse]. Authentication and disconnect reason codes
distinguish a rejected pairing from a failed link or missing HID subscription.
Do not clear all firmware settings or erase the bond database as a first step.
The device displays PAIR FAILED for authentication failure, LINK LOST for a
disconnection, and HID WAIT for an encrypted link without HID readiness.
The host tests verify these state transitions, not Windows interoperability.

1. With Bluetooth off, verify Air Mouse shows BT OFF and BadUSB BLE explains
   that Bluetooth must be enabled. USB BadUSB and PC Monitor must still work.
2. Enable Bluetooth. Pair Air Mouse with your own PC. Verify movement, drag,
   right-click and scrolling. Long-press B while dragging: no stuck button.
3. Open BadUSB BLE with a harmless text-only test on your own PC. Verify the
   READY state, modifiers and released keys. Disconnect during input: execution
   must stop and must not resume by itself after reconnecting.
4. Reopen each app and reconnect using saved pairings. Repeat Air Mouse/BadUSB
   transitions at least 100 times. Record free/minimum heap, resets and errors.
5. Enter and exit app07 without starting advertising, then retest both HID apps.
   No protocol/crash/flood testing is required for this integration.
6. Toggle system Bluetooth off/on, reboot with each saved setting, and repeat
   pairing on Windows 10/11. Verify Wi-Fi operation and USB PC Monitor afterward.

Do not publish this integration as hardware-validated until these checks pass.
