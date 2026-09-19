# VelaGesture — Hardware Adaptation

## Target Hardware

**Board:** ESP32-P4 Function-EV-Board V1.6
**Chip:** ESP32-P4, revision v3.2

## Hardware Specifications

| Component     | Specification                      |
|---------------|------------------------------------|
| CPU           | Dual-core RISC-V, 400 MHz          |
| PSRAM         | 32 MB                              |
| Flash         | 16 MB (external)                   |
| Display       | 1024x600 MIPI-DSI                  |
| Camera        | MIPI-CSI (OV2640/OV5640)           |
| Touch         | I2C capacitive touch controller    |
| Audio         | I2S codec (mic + speaker)          |
| Connectivity  | Wi-Fi 6, BLE 5                     |

## Pin / Interface Mapping

### Camera (MIPI-CSI)

The MIPI-CSI interface is configured by the board BSP.  No pin
remapping is required.  The camera device is accessible at
`/dev/video0` after the board initialises.

### Display (MIPI-DSI)

The 1024x600 MIPI-DSI display is initialised by the board BSP.
LVGL is configured to use the framebuffer at `/dev/fb0`.

### Audio (I2S)

The I2S audio codec provides:
- Microphone input: `/dev/audio/pcm_in0`
- Speaker output: `/dev/audio/pcm_out0`

Audio format: 16 kHz, 16-bit, mono.

### Touch Panel

The I2C capacitive touch controller is at `/dev/input0`.
Not directly used by VelaGesture but available for UI navigation.

## Board BSP Status

The ESP32-P4 Function-EV-Board BSP is maintained in the openvela
vendor repository at `vendor/espressif/`.  The BSP provides:

- CPU and memory initialisation
- MIPI-CSI camera driver
- MIPI-DSI display driver
- I2S audio codec driver
- I2C touch controller driver
- GPIO, SPI, UART peripherals
- Wi-Fi and BLE stack

VelaGesture does not modify the BSP.  It uses the standard NuttX
device interfaces.

## Board Configuration

The board configuration used for the contest build is:

```
vendor/espressif/boards/esp32p4/ev/configs/vela/
```

This configuration enables:
- RISC-V dual-core SMP
- 32 MB PSRAM
- MIPI-CSI camera
- MIPI-DSI display with LVGL
- I2S audio
- Wi-Fi networking
- NuttX shell (NSH)

## Adaptation Notes

1. **No custom BSP modifications** — VelaGesture runs entirely
   on top of the standard BSP using NuttX device interfaces.

2. **Camera inference** — the inference backend (ESP-DL or
   TF-Lite Micro) uses the ESP32-P4's hardware acceleration
   where available.  The fallback path works on any P4 board
   configuration.

3. **Audio** — the audio subsystem is optional
   (`CONFIG_VELAGESTURE_AUDIO`).  If the audio codec driver is
   not configured for a particular board variant, the application
   starts without audio.

4. **Display** — LVGL rendering is not a core part of VelaGesture's
   functionality.  Status information is primarily shown on the
   serial console.  A future version may add an LVGL UI.
