# PC Monitor

Keep your PC's temperature, load and memory use in view on MeowKit.
Connect a USB data cable, start HardwareSerialMonitor on Windows, and open
**Apps > PC Monitor**.

![PC Monitor interface](pc-monitor-preview.png)

Actual LVGL host render at 320 × 240, using sample values from the maintainer's
test. This preview is not a photograph of the updated firmware on hardware.

## Windows client

[Download HardwareSerialMonitor for MeowKit](../software/HardwareSerialMonitor/downloads/HardwareSerialMonitor-v1.4.4-MeowKit.zip)

Extract the complete ZIP, run `HardwareSerialMonitor.exe` as administrator,
and choose MeowKit's COM port from its Windows notification-area menu.
Check hidden tray icons if you do not see it. The client uses .NET Framework 4.8
and 9600 baud, 8 data bits, no parity, 1 stop bit.

Use MeowKit in normal operating mode. Download mode is only for firmware
flashing. Close any serial monitor using the same COM port.

The package includes the original v1.4.4 program, runtime dependencies,
matching-version source project and license. See
[client setup and build instructions](../software/HardwareSerialMonitor/README.md).

## Read the screen

The two upper cards show CPU and GPU information. Long names scroll within
their cards. Each card shows temperature in degrees Celsius and load in percent.
The CPU card also shows clock speed in MHz; the GPU card shows total VRAM in GB.

The lower card shows RAM used / total GB. All five bars follow received data.
Load and RAM bars use 0–100%; temperature bars use 0–100 degrees Celsius and
cap visually at 100, while the numeric value can exceed it. Bar colors change
at 70 and 90; they are visual thresholds, not hardware-specific alarm limits.

- **Connect USB – start the PC client:** waiting for the first complete sample.
- **Live:** valid packets are arriving.
- **Data stopped:** no complete packet for five seconds; old readings are cleared.
- **--:** the client did not provide a usable value for that field.

Hold **B** to exit. The client can stay open on Windows.

## Troubleshooting

If all values are missing, check the cable, selected COM port and client tray
icon. Close other applications using that port. Reselect the port after
reconnecting if Windows assigned a different one.

If only a temperature or GPU reading is missing, check client permissions and
hardware support. This legacy sensor library may not recognize newer hardware.
The MeowKit display cannot generate a reading that the client does not send.

## Source map

| Path | Purpose |
| --- | --- |
| `src/app/app_05/pc_monitor.cpp` | App lifecycle, serial receive, display updates and stale-data state |
| `src/app/app_05/pc_monitor_protocol.h` | Fixed-size, host-testable HSM parser |
| `src/app/app_05/asset/ui_PC_Monitor.c` | Native LVGL 320 × 240 layout |
| `src/app/app_05/asset/` | UI globals, built-in font and original image resources |
| `software/HardwareSerialMonitor/source/` | Vendored Windows project and resources |
| `software/HardwareSerialMonitor/downloads/` | User download ZIP including source |
| `test/pc_monitor_protocol_test.cpp` | Protocol regression tests |
| `test/pc_monitor_ui/` | Real LVGL host rendering and lifecycle checks |

The app does not read images from an SD card. Original artwork stays in app05
for reference; the new screen uses native widgets and the existing numeric font.

## Serial protocol

The supplied v1.4.4 client calls `SerialPort.WriteLine()` and uses pipe-separated
fields. A representative sample is:

```text
C36c 13%|G44c 46%|R13.8GB|RA2.2|RL86|GMT12288|GMU1000|GML8|GFANL0|GRPM0|GPWR30|CPU:Intel Core i7-7700KGPU:NVIDIA GeForce RTX 4070 SUPER|GCC2500||GMC10000||GSC0||CHC1600|
```

`C` and `G` carry temperature and load; `R` is RAM used; `RA` is RAM available.
Total RAM is their sum. `GMT` is total VRAM in MB; `CHC` is CPU clock in MHz.
CPU and GPU names may share a token or arrive as separate tokens.

The receiver publishes after a newline or the next CPU sample. For clients
without a newline, it also supports a 150 ms idle gap following a delimiter.
CPU, GPU, RAM-used and RAM-available fields must all have arrived. Numeric
fields accept dot or comma decimals; unavailable, non-finite and out-of-range
values appear as `--`. Unknown fields are ignored. An overlong token invalidates
the current sample, and the next CPU sample resynchronizes reception.

Each token is limited to 255 bytes; names to 95 bytes. Reception processes at
most 512 bytes per app loop without sleeping between fields. The launcher
continues polling B while data arrives. Stale detection uses unsigned elapsed
time, including across the `millis()` wrap-around.

## Validation

Run protocol tests with a host C++17 compiler:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic test/pc_monitor_protocol_test.cpp -o output/pc_monitor_protocol_test.exe
./output/pc_monitor_protocol_test.exe
```

Render the actual LVGL screen (GCC example on Windows):

```powershell
python tools/test_pc_monitor_ui.py --cc C:/msys64/mingw64/bin/gcc.exe
```

This writes preview BMPs to ignored `output/`, checks text widths and the degree
glyph, and opens/closes the screen 30 times. Build the firmware with `pio run`.

Hardware acceptance after flashing:

1. Connect the supplied Windows client; compare CPU/GPU values with its source sensors.
2. Check the full long model names, `°C`, `100%`, RAM units, clock and VRAM.
3. Stop the client; confirm readings clear after five seconds. Restart it and
   confirm automatic recovery without restarting MeowKit.
4. Hold B while receiving data; reopen PC Monitor repeatedly and check stability.
5. Unplug/reconnect USB and test an unsupported sensor (`--`).

Host rendering and firmware compilation do not replace this on-device test.
