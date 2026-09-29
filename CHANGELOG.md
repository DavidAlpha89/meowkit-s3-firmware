# Changelog

All notable changes to MeowKit-S3 firmware are documented here.

## [Unreleased]

### PC Monitor

- Restored the original two-layer artwork in Flash, with live segmented meters,
  CPU/GPU clock speeds, RAM usage, GPU fan load and RPM; no SD-card image reads.
- Removed obsolete app05 gauge/background arrays and the unused duplicate system
  background; retained the original PNGs and added a repeatable asset converter.
- Added live LVGL meters with a five-color scale: cyan, sky blue, violet,
  amber, and coral red, including the vertical used-memory meter.
- Fixed missing degree glyphs, overflowing hardware names and ambiguous RAM/clock labels.
- Added waiting, live and five-second stale-data states; unavailable readings show `--`.
- Replaced delimiter-triggered partial parsing and blocking delays with a bounded
  stream parser, packet validation, decimal-comma support and malformed-input recovery.
- Released app-owned LVGL objects on exit to prevent repeated-open memory growth.
- Added the user-tested HardwareSerialMonitor v1.4.4 Windows download, matching-version
  source snapshot, original resources, provenance and setup/build documentation.
- Added host protocol regressions and actual LVGL layout/lifecycle checks.

PC Monitor operation was confirmed on hardware by the maintainer. Host tests and
firmware build results are recorded in the PC Monitor guide. This is a source
update, not a new firmware release.

### VU Meter

- Embedded both VU Meter images in firmware Flash so the app no longer needs
  those files on the SD card.
- Composited the needle and mask in one frame to prevent mask flicker while the
  needle responds to sound.
- Removed the obsolete SD-card copies and cleaned up app resources on exit.

## [1.0.1] - 2026-09-23

### Added

- Added synchronized startup audio for `boot_animation.gif`.
- Added button sounds for the joystick and A/B buttons.
- Added an asynchronous system audio service with persisted volume, Key Tone
  control, amplifier sequencing, and audio-device suspend/resume.
- Added pressed-state feedback to the Settings, MSC, and Info buttons.
- Added an About dialog showing firmware version `v1.0.1` and author Mingo.
- Added system audio architecture and hardware acceptance-test documentation.

### Improved

- Improved ES8311 output with dedicated MCLK, APLL clocking, controlled amplifier
  sequencing, and reliable 44.1/48 kHz switching.
- Matched startup audio length to the GIF automatically. Only complete MP3 frames
  needed by the animation are embedded, reducing the current boot sound payload
  from about 2.3 MB to 74.1 KB.
- Refined BLE Spam joystick and A/B interaction while preserving its TUI design.
  Removed the external `attacking.png` runtime dependency.
- Vendored required libraries under `lib/`; a normal clone now contains the
  source dependencies without Git submodule initialization.
- Added a contributor workflow covering implementation, review, testing,
  attribution, merge, and post-merge verification.

### Fixed

- Prevented launcher reboots caused by overlapping LVGL transitions during rapid
  joystick navigation. See [PR #29](https://github.com/mingolucky/meowkit-s3-firmware/pull/29).
- Fixed navigation dispatch and transition ownership across persistent screens
  and applications.
- Fixed Wi-Fi status marquee overflow and Wi-Fi Connect label alignment.
- Fixed app03 VU Meter exit causing a loud speaker burst and leaving system audio
  unavailable. ES7210 input and ES8311 output now release and reclaim their
  shared MCLK/BCLK/WS pins in a defined order.
- Fixed ES8311 gain mapping that could make system sounds inaudible.
- Fixed MCLK mismatch when switching between the 44.1 kHz startup sound and
  48 kHz button sound.
- Fixed long-press B ownership in BLE Spam so the launcher closes the app and
  releases Bluetooth resources consistently.

### Removed

- Removed empty application placeholders `app_10` through `app_15`.
- Removed their unused App Menu icons and registration entries. The menu now
  exposes the nine implemented applications only.
- Excluded generated `output/`, build products, and local editor files from
  repository tracking.

### Validation

- PlatformIO release build for `esp32s3box`: passed.
- Final build usage: 126,124 bytes RAM (38.5%) and 7,260,073 bytes Flash (55.4%).
- Firmware compilation, device download, and hardware functional testing passed
  for the v1.0.1 release candidate.
- Verified release paths include rapid navigation, app03 exit/audio recovery,
  system volume and Key Tone behavior, and application entry/exit.

### Credits

- Firmware and product integration: Mingo.
- Screen-transition stability: [@warengonzaga](https://github.com/warengonzaga),
  with upstream credit to [@janud](https://github.com/janud).

## [1.0.0]

- Initial public release of MeowKit-S3 firmware.
