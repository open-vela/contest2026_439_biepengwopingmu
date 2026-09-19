# VelaGesture — Build Instructions

## Prerequisites

- Linux host (Ubuntu 22.04+ recommended)
- ESP32-P4 toolchain (riscv-none-elf-gcc)
- openvela repo tool
- ESP-IDF v5.x (for ESP32-P4 flash tools)

## Step 1: Fetch the Workspace

```bash
# Create a working directory
mkdir -p ~/velaworkspace && cd ~/velaworkspace

# Initialise repo with the contest manifest
repo init -u https://github.com/open-vela/contest2026_439_biepengwopingmu \
  -b dev-ai-contest-2026 -m contest2026_439_biepengwopingmu.xml

# Sync all repositories
repo sync -c -j8
```

After sync, the workspace structure is:

```
~/velaworkspace/
  nuttx/                     # NuttX kernel
  apps/                      # NuttX applications
  packages/                  # Packages (demos, apps, ai_agent)
  vendor/                    # Vendor BSPs
  frameworks/                # Frameworks (graphics, multimedia, etc.)
  external/                  # Third-party libraries
  contest2026_439_biepengwopingmu/  # This repo (your code)
  build.sh -> nuttx/tools/build.sh  # Build entry point
```

## Step 2: Enable VelaGesture

```bash
cd ~/velaworkspace

# Open menuconfig
./build.sh vendor/espressif/boards/esp32p4/ev/configs/vela menuconfig
```

Navigate to:

```
Application Configuration
  → Demos
    → [*] VelaGesture multimodal gesture interaction terminal
```

Configure WebSocket server address:

```
Application Configuration
  → Demos
    → VelaGesture multimodal gesture interaction terminal
      → Guxian WebSocket server host: 192.168.1.100
      → Guxian WebSocket server port: 8443
```

Optionally enable audio:

```
      → [*] Enable audio capture and playback
```

Save and exit menuconfig.

## Step 3: Build

```bash
cd ~/velaworkspace
./build.sh vendor/espressif/boards/esp32p4/ev/configs/vela -j8
```

The build produces:

```
out/esp32p4-ev/vela/
  nuttx.bin          # Firmware binary
  nuttx.elf          # ELF with debug symbols
```

## Step 4: Flash

Connect the ESP32-P4 Function-EV-Board via USB and flash:

```bash
cd ~/velaworkspace

# Flash using esptool (via ESP-IDF)
esptool.py --chip esp32p4 \
  --port /dev/ttyUSB0 \
  --baud 921600 \
  write_flash 0x0 out/esp32p4-ev/vela/nuttx.bin
```

Alternatively, if using the openvela flash helper:

```bash
./build.sh vendor/espressif/boards/esp32p4/ev/configs/vela flash
```

## Step 5: Install the Skill

After flashing, copy the skill definition to the device:

```bash
# Via ADB (if available)
adb push skills/guxian-control/ /data/agent/skills/guxian-control/

# Or via NSH mount + copy
# (depends on your host-to-device file transfer method)
```

## Step 6: Run

Open a serial console and launch the application:

```bash
# Connect serial console
screen /dev/ttyUSB0 115200

# In the NSH prompt:
nsh> velagesture
```

Expected output:

```
==========================================
  VelaGesture v1.0.0
  Multimodal Gesture Interaction Terminal
  Platform: ESP32-P4 + openvela
==========================================

[INIT] Application context ready
[INIT] Skill executor ready
[INIT] WebSocket client connected
[INIT] AI agent started
[INIT] Gesture perception started
[RUN] VelaGesture is running. Press Ctrl+C or send SIGINT to stop.
```

## Build Without Audio

If the audio driver is not available on your board configuration,
leave `CONFIG_VELAGESTURE_AUDIO=n` (the default).  The application
will run without audio capture/playback.

## Troubleshooting

- **"Cannot open /dev/video0"** — ensure the camera driver is
  enabled in menuconfig and the camera is physically connected.
- **"WebSocket not connected"** — the device will run in offline
  mode.  Check the server address and network configuration.
- **"Skills directory not found"** — install the skill file as
  described in Step 5.  The built-in guxian-control fallback
  allows basic operation even without the on-device SKILL.md.
