# Source and binary provenance

## Snapshot

- Original project: https://github.com/koogar/HardwareSerialMonitor
- Public source mirror: https://github.com/Len-china/HardwareSerialMonitor
- Mirror commit: `5560f0b0ddcb27db3aae6420b3d6247697e48e63` (2023-09-28).
- Subdirectory: `OHM_Based/HardwareSerialMonitor_v1.4.4_9600_OHM.0.9.6.1.PR_NET4.8_SRC`.
- Supplied archive: `HardwareSerialMonitor.7z` (provided by MeowKit's maintainer).
- Version identified by About dialog text: **v1.4.4, 9600 baud, .NET 4.8**.
  The EXE's Windows file-version resource says **1.1.0.0**; these differ upstream.

## Verified matches

The supplied executable matches the mirror's
`HardwareSerialMonitor/obj/Release/net48/HardwareSerialMonitor.exe` byte for byte:

```text
SHA256 30765564bf2f6cc7021d58f203d53bf3ea9dc376a3b90f1617726061175f4c4b
```

The supplied `OpenHardwareMonitorLib.dll` matches the project's referenced DLL:

```text
SHA256 c0c0c4b4e0c5958c2d6608b94c32c52107576fb34504820caa9b191f15757414
```

This establishes binary identity with the archived project output, not a
reproducible-build claim. The desktop application has not been rebuilt here.

## Packaging

The runtime EXE and DLLs are unchanged. MeowKit adds a relative-path launcher,
instructions and source. Original per-PC Config.ini, installer/uninstaller files,
debug symbols, build output and Visual Studio user settings are excluded.
The original archive remains untouched on the maintainer's machine.

Rebuild the download with Python and `py7zr`:

```powershell
python tools/package_hardware_serial_monitor.py PATH_TO_ORIGINAL_ARCHIVE.7z
```

## Third-party notices

- HardwareSerialMonitor: Colin Conway, Rupert Hirst and contributors;
  supplied GPL license and attribution retained with source.
- OpenHardwareMonitorLib: upstream sensor library, version identified by the
  snapshot as 0.9.6.1 pre-release. See `source/0.9.6.1-Pre.txt` for upstream
  development references; its source is separate from the Windows client.
- INIFileParser and System.CodeDom: unchanged runtime dependencies from the
  supplied package. The client project declares its NuGet dependencies.

No Windows installer, driver installation or system configuration is executed
by the packaging script. The supplied EXE was not launched during this review.
