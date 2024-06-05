# Compilation

Requirements:
- Arduino IDE
- Attiny support
- RadioHead library

## Add attiny support
- Copy the following link to the **File** > **Preferences** > **Additional boards manager URLs**:
`https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json`

## Download and install RadioHead library
1. Download [RadioHead](http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.130.zip)
0. Install - **Sketch** > **Include Library** > **Add .ZIP Library**

## Set right compilation values:
- **Tools** > **Board** and **Processor** to `attiny85`
- **Tools** > **Clock** to `Internal 8 MHz` (1 MHz doesn't work due to issues with RadioHead library - see RH_ASK.h comments)
- **Tools** > **Programmer** to `Arduino as ISP`

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
- **Sketch** > **Upload Using Programmer**

