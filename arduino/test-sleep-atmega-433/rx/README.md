
# Compilation

Requirements:
- Arduino IDE
- Raspberry Pi Pico W support
- RadioHead library

## Add Raspberry Pi Pico W support
- Copy the following link to the **File** > **Preferences** > **Additional boards manager URLs**:
`https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

- Open **Tools** > **Board** > **Boards Manager** and install **Raspberry Pi Pico** by **Earle F. Philhower, III**

## Download and install RadioHead library
1. Download [RadioHead](http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.130.zip)
0. Install - **Sketch** > **Include Library** > **Add .ZIP Library**

## Set right compilation values:
- **Tools** > **Board** to `Raspberry Pi Pico/RP2040`>`Raspberry Pi Pico W`
- **Tools** > **Port** to `/dev/ttyACM0`
- **Tools** > **CPU Speed** to `133 MHz` (default)

## Fix problem with compilation using dirty hack that will damage the RH library
- Open library location (`~/Arduino/libraries/RadioHead/`) and delete the following files:
```
RH_RF69.cpp
RH_RF69.h
RH_STM32WLx.cpp
RH_STM32WLx.h
RH_SX126x.cpp
RH_SX126x.h
```

## Compile and upload
- **Sketch** > **Upload**
