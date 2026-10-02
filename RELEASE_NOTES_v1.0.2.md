# MeowKit-S3 firmware v1.0.2

This release brings the Desktop Gadgets updates into the official firmware and
improves the applications you use every day.

## What changed

- **VU Meter:** both artwork layers now live in Flash. The needle and mask are
  drawn together to remove the visible flicker.
- **PC Monitor:** a clearer live dashboard with five-color meters, readable
  temperature and memory values, and explicit waiting/stale states. The
  matching Windows HardwareSerialMonitor package and source are in
  `software/HardwareSerialMonitor/`.
- **Air Mouse:** improved gyro calibration, clicks and scrolling. The current
  Air Mouse build was confirmed working by the maintainer. Its Bluetooth
  connection remains independent of the Settings Bluetooth switch.
- **Infrared:** remote folders, save location selection, append confirmation
  and safer deletion and file parsing.
- **BadUSB BLE and Bluetooth:** app-owned sessions and shutdown cleanup were
  added for BadUSB. BLE scripts wait for the host to subscribe before reporting
  ready and stop if the connection is lost.

## Install

Use the [browser installer](https://github.com/mingolucky/meowkit-s3-installer)
on a desktop Chrome or Edge browser, or download the factory image from that
repository. Hold BOOT while powering on MeowKit, then connect it with a USB
data cable. An erase operation also removes saved settings.

For documentation, visit [docs.meowkit.cc](https://docs.meowkit.cc/).

## Validation

The `esp32s3box` PlatformIO build and host-side regression tests passed.
Air Mouse and PC Monitor were confirmed by the maintainer on hardware.
Infrared hardware operations, BadUSB BLE pairing and prolonged app switching
still need wider device testing. See [CHANGELOG.md](CHANGELOG.md) for details.

Infrared changes credit
[@warengonzaga](https://github.com/warengonzaga/FeralCat). Confirm upstream
redistribution terms before using those changes outside this repository.
