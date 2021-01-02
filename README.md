View this project on [CADLAB.io](https://cadlab.io/project/23184). 

(this project is not dead, just paused due to financial reasons)

~~[2019-03-12] Current roadmap is to create (and maybe sell) a drone-specific board for the RaspberryPi Compute Module, allowing reduced weight by stacking every peripherals on it. This also enables the possibility to use an STM32Fx to control IMU and motors, as the RaspberryPi's DMA can only produce 500Hz PWM but not oneshot/multishot/dshot~~

[2020-11-05] Since Raspberry Pi Foundation just released a new Compute Module with a smaller form factor, better CPU and 4 PWM outputs, it is now possible to have a very small looptime and high frequency motors update directly on the Pi itself. I'm now working on a carrier board with all the necessary hardware (gyroscope, accelerometer, magnetometer, barometer, 4 PWM outputs, radio modules [RFM95W with external antenna plugs] & SBUS input [with dedicated uart inverter], ADC, USB & UART pins [for GPS/other modules], I2C pins [for external sensors], TVOUT [compatible with any VTXs], 4 GPIO pins, dual camera port, microSD plug, 5V3A UBEC). Total size is 66x40mm

<img src="https://raw.githubusercontent.com/dridri/bcflight/master/images/cm4-fc-front.png" width="49%" /> <img src="https://raw.githubusercontent.com/dridri/bcflight/master/images/cm4-fc-back.png" width="49%" />

[2021-01-02] The new Raspberry Compute Module 4 actually does not expose the 2 extra PWM. As a workaround, a cheap but powerful IC (STM32G474CE) will be used as PWM translator. This will allow an update frequency up to 8kHz with only 1 core used on the CPU.
Once tested and validated, the circuit design will be available for free on this repo.

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

This video was recorded on 22-nov-2018 during an FPV session

<img width="512" alt="FPV Video of drone" src="https://i.ibb.co/NF0CKLV/drich.gif"/>

## Latest videos :
* [2017-08-11] https://www.youtube.com/watch?v=-q8AbUXIIAU
* [2017-08-11] https://www.youtube.com/watch?v=f2EZNX43WbE
* [2018-04-13] https://www.youtube.com/watch?v=BVB1eS7WqcU

# Current test setup

Racer :

<a href="https://ibb.co/N1bwd0m"><img src="https://i.ibb.co/vsR26NQ/20200426-140237.jpg" alt="20200426-140237" border="0" /></a>

(Black coating on the PCB is Plasti-Dip, protecting it from dust and moisture. More recently, the Raspberry was coated with KF-1282 anti-corrosion spray)
* ZMR250 frame
* Raspberry Pi 3 A+
* Raspberry Pi Camera v2.1 (8MP, Sony IMX219 sensor) + M12 wide-angle lens
* RFM95W radio module (SX1276-like 20dBm chip)
* MPU9150 (gyroscope + accelerometer + magnetometer)
* ADS1015 (ADC/Voltmeter)
* ACS709 (current sensor)
* 4x Multistar BLHeli-S 30A ESCs
* 4x Multistar Elite 2204-2300KV motors

RC controller :
* Taranis X9D empty case
* Taranis X9D gimballs
* Raspberry Pi 3
* Waveshare 3.5" HDMI touchscreen
* inAir9B radio module (SX1276 20dBm chip)
* MCP3208 ADC (for gimballs input and battery voltage sensing)
* 10x ON-ON switches
