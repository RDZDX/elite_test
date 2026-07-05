# Elite for Nokia 225 (MRE Port). Alpha version. The program only works partially.

A port of [**elite-ce**](https://github.com/ariajd27/elite-ce) — a C reimplementation of the classic space trading game *Elite* — to the **MediaTek Runtime Environment (MRE)** platform, targeting the **Nokia 225** feature phone as a `.vxp` application.

---

## What is this?

**elite-ce** was originally written for the TI-84 Plus CE calculator using the CE C/C++ toolchain and TI-proprietary APIs (`graphx.h`, `keypadc.h`, `fileioc.h`). This port replaces all platform-specific code with MRE C API calls while keeping all game logic intact.

The project structure follows [**XimikBoda/CmakeMreTemplate**](https://github.com/XimikBoda/CmakeMreTemplate).

---

## Dependencies

| Tool | Purpose |
|------|---------|
| **MRE C API SDK** | Headers (`vmgraph.h`, `vmio.h`, `vmsys.h`, etc.) and `.a` libs for ARM |
| **TinyMRESDK** | Packs compiled ELF + resources into a `.vxp` app bundle |
| **arm-none-eabi-gcc** | ARM cross-compiler (GCC 9+) |
| **CMake 3.10+** | Build system |

---

## Build Instructions

### 1. Prerequisites

- Install `arm-none-eabi-gcc` (e.g. `sudo apt install gcc-arm-none-eabi`)
- Obtain the MRE C API SDK and TinyMRESDK
- Set environment variables (or pass as CMake args):

```sh
export MRE_SDK=/path/to/mre-sdk
export TinyMRESDK=/path/to/TinyMRESDK
```

### 2. Configure and Build

```sh
# Release build (ARM, Unix/Linux host)
cmake --preset arm-release-unix \
      -DMRE_SDK=$MRE_SDK \
      -DTinyMRESDK=$TinyMRESDK

cmake --build build/arm-release-unix
```

The output `.vxp` will be in `build/arm-release-unix/main/`.

### Available CMake Presets

| Preset | Description |
|--------|-------------|
| `arm-release` | ARM release (Windows host) |
| `arm-debug` | ARM debug (Windows host) |
| `arm-release-unix` | ARM release (Linux/macOS host) |
| `arm-debug-unix` | ARM debug (Linux/macOS host) |
| `win32` | Win32/MoDis simulation build |

---

## Nokia 225 Key Mapping

| Game Action | Nokia 225 Key | MRE Keycode |
|-------------|--------------|-------------|
| Throttle up | `#` key | `VM_KEY_POUND` |
| Throttle down | `1` key | `VM_KEY_NUM1` |
| Pitch up | Up arrow | `VM_KEY_UP` |
| Pitch down | Down arrow | `VM_KEY_DOWN` |
| Roll left | Left arrow | `VM_KEY_LEFT` |
| Roll right | Right arrow | `VM_KEY_RIGHT` |
| Fire laser | `0` key | `VM_KEY_NUM0` |
| Arm/fire missile | `5` key | `VM_KEY_NUM5` |
| Disarm missile | `4` key | `VM_KEY_NUM4` |
| View direction | Right soft key + arrows | `VM_KEY_RIGHT_SOFTKEY` |
| Open menu | Left soft key | `VM_KEY_LEFT_SOFTKEY` |
| Jump / Hyperspace | `*` (hold) | `VM_KEY_STAR` |
| Galactic jump | `2` (hold) | `VM_KEY_NUM2` |
| In-system jump | `3` key | `VM_KEY_NUM3` |
| ECM | `6` key | `VM_KEY_NUM6` |
| Delete (name entry) | `7` key | `VM_KEY_NUM7` |
| OK / Confirm | Center/OK key | `VM_KEY_OK` |

---

## Installation

1. Copy the built `.vxp` file to the **SD card** of your Nokia 225.
2. On the phone, navigate to the SD card via the file manager.
3. Open the `.vxp` file — the phone will prompt to install it.
4. After installation, launch **Elite** from the applications menu.

The save file is stored at `E:\elite.sav` on the SD card.

---

## Screen Layout

The game runs at **320×240** pixels (landscape), rendered via MRE layer rotation.

- **View area**: 320×210 pixels (top portion) — 3D space view
- **Dashboard**: 320×30 pixels (bottom strip) — ship status, radar, gauges

---

## Credits

- Original *Elite* — David Braben & Ian Bell (1984)
- **elite-ce** port — [ariajd27](https://github.com/ariajd27/elite-ce)
- MRE port — RDZDX
- CMake MRE template — [XimikBoda](https://github.com/XimikBoda/CmakeMreTemplate)
- TinyMRESDK — [XimikBoda](https://github.com/XimikBoda/TinyMRESDK)
