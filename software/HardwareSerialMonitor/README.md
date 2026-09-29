# HardwareSerialMonitor for MeowKit

Send PC temperatures, load and memory readings to **Apps > PC Monitor** over USB.

## Download

[Download the Windows package](https://github.com/mingolucky/meowkit-s3-firmware/raw/refs/heads/main/software/HardwareSerialMonitor/downloads/HardwareSerialMonitor-v1.4.4-MeowKit.zip)

The ZIP includes the user-tested HardwareSerialMonitor v1.4.4 executable,
runtime dependencies, license, and the matching-version desktop project source.
This is an upstream compatibility package, not a new MeowKit-authored Windows app.

## Connect

1. Extract the entire ZIP to a writable folder on your Windows PC.
2. Turn MeowKit on normally and open **Apps > PC Monitor**.
3. Connect a USB **data** cable. Keep MeowKit in its normal operating mode.
4. Run `HardwareSerialMonitor.exe` as administrator. Keep its DLL files beside it.
5. Open its icon in the Windows notification area (including hidden icons),
   then select MeowKit's COM port. The original client uses **9600 baud, 8N1**.
6. Wait for **Live** on MeowKit. Hold **B** to return to the Apps menu.

`Start.cmd` starts the client with its own folder as the working directory.
The client stores the selected device in a new local `Config.ini`.
No device selection from the original owner's PC is included.
The upstream program targets .NET Framework 4.8.

## Read the display

- CPU/GPU: temperature in degrees Celsius and load in percent.
- CPU clock: MHz. GPU memory: total VRAM in GB.
- RAM: used / total GB, plus a live usage bar.
- Long hardware names scroll inside their cards.
- `--`: no usable reading was provided for that field.
- **Data stopped**: no complete packet has arrived for five seconds. Readings
  clear automatically and resume when valid data returns.

Sensor availability depends on your CPU, GPU, driver and this legacy client's
sensor library. An unsupported temperature sensor cannot be fixed by the display.
Temperature bars cap at 100 degrees Celsius; numeric readings can go higher.

## If no readings appear

Close VS Code/PlatformIO Serial Monitor and other software using the same COM
port. Confirm the selected device in Windows Device Manager, check the data
cable, and reselect the port after reconnecting. If only some readings are
missing, check the client permissions and sensor support.

## Source and build

- Device application and compiled-in UI: `../../src/app/app_05/`.
- Windows client source: `source/HardwareSerialMonitor/`.
- Provenance, version and hashes: [PROVENANCE.md](PROVENANCE.md).
- MeowKit guide and test procedure: `../../docs/pc-monitor.md`.

Open `source/HardwareSerialMonitor.sln` with Visual Studio 2022 and the .NET
desktop development workload plus the .NET Framework 4.8 targeting pack.
Restore NuGet dependencies and build Release. Alternatively, with a .NET SDK
and the targeting pack installed:

```powershell
dotnet build source/HardwareSerialMonitor/HardwareSerialMonitor.csproj -c Release
```

The SDK-style project restores additional packages from NuGet. This is not an
offline desktop build. The shipped EXE is the supplied upstream binary;
it has not been rebuilt or modified in this project.

## Attribution

HardwareSerialMonitor: Colin Conway, Rupert Hirst and contributors. Its bundled
GPL license is preserved in `source/LICENSE.txt` and in the download package.
Third-party dependencies retain their original licenses. See PROVENANCE.md.
