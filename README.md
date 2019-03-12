(this project is not dead, just paused due to financial reasons)

[12-03-2019] Current roadmap is to create (and maybe sell) a drone-specific board for the RaspberryPi Compute Module, allowing reduced weight by stacking every peripherals on it. This also enables the possibility to use an STM32Fx to control IMU and motors, as the RaspberryPi's DMA can only produce 500Hz PWM but not oneshot/multishot/dshot

# BeyondChaos Flight
This project aims to create open-source flight controller and ground controller for Linux based drones.
The general idea is to allow to create any kind of flying object, from quad-racer to autonomous flying wing.

## flight
The 'flight' subfolder contains the flight controller itself, currently supporting only Raspberry Pi board, but can be easily extended to any kind of board (including non-Linux ones, but it is not the purpose of this project).

## controller_pc
Highly configurable GUI client that allow you to access any controls over flight controller, see livestream, and fly it.

## controller_rc
This subfolder contains a ground controller designed to be used in an RC remote control. Currently only supporting RaspberryPi board.

## fpviewer
Livestream viewer supporting stereo mirroring and head-up display overlay. Designed to be used with any HDMI goggles (tested with OSVR).

## libcontroller
Contains everything needed to communicate with the drone from ground.

## librawwifi
This is inspired from befinitiv's wifibroadcast, it allows to directly use WiFi in analog-like transmission.

Alternatively, hostapd can be used on the drone and wpa_supplicant on the controller to allow TCP/UDP/UDPLite communication.

## tools
Generation scripts for easier building

## buildroot
*in progress*
Contains buildroot configuration for both flight and ground targets.

# Results

This video was recorded on 03-feb-2017 during an FPV session
[![FPV Video of drone](http://img4.hostingpics.net/pics/934968drone20170302.gif)](https://drive.google.com/file/d/0Bwo1JutVEkplLV9DNU5hZEtFcnM/view?usp=sharing)

## Latest videos :
* [2017-08-11] https://www.youtube.com/watch?v=-q8AbUXIIAU
* [2017-08-11] https://www.youtube.com/watch?v=f2EZNX43WbE
* [2018-04-13] https://www.youtube.com/watch?v=BVB1eS7WqcU

# Current test setup

![Photo of drone and controller](http://img4.hostingpics.net/pics/982150drichdronezmrcontrollersmall.jpg)

Racer :
* ZMR250 frame
* Raspberry Pi 3
* Raspberry Pi official camera v2.1 (8MP, Sony IMX219 sensor) + wide-angle lens
* inAir9B radio module (SX1276 20dBm chip)
* MPU9150 (gyroscope + accelerometer + magnetometer)
* ADS1015 (ADC/Voltmeter)
* ACS709 (current sensor)
* 4x DYS XM20A ESCs (flashed with BLHeli, DampedLight enabled)
* 4x Multistar Elite 2204-2300KV motors

RC controller :
* Taranis X9D empty case
* Taranis X9D gimballs
* Raspberry Pi 3
* Waveshare 3.5" touchscreen
* inAir9B radio module (SX1276 20dBm chip)
* MCP3208 ADC (for gimballs input and battery voltage sensing)
* 10x ON-ON switches
