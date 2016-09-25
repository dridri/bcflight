# BeyondChaos Flight
This project aims to create open-source flight controller and ground controller for Linux based drones.

## flight
Current status : flying - beta

The 'flight' subfolder contains the flight controller itself, currently supporting only Raspberry Pi board, but can be easily extended to any kind of board (including non-Linux ones, but it is not the purpose of this project).

## controller_rpi
Current status : ok

This subfolder contains a ground controller specifically designed for Raspberry Pi.

## librawwifi
Current status : ok

This is a fork of wifibroadcast from befinitiv, it allows to directly use WiFi in analog-like transmission.

Alternatively, hostapd is used on the drone and wpa_supplicant on the controller to allow socket communication.

## tools
Current status : ok

Generation scripts for easier building

## buildroot
Current status : in progress

Contains buildroot configuration for both flight and ground targets.


# Current test setup

![Photo of drone and controller](https://dl.dropboxusercontent.com/u/81758777/drich-drone-zmr-controller-small.jpg)

Racer :
* ZMR250 frame
* Raspberry Pi Zero v1.2
* Raspberry Pi official camera v2.1 (8MP, Sony IMX219 sensor) + wide-angle lens
* Alfa Network AWUS036NH (pushed to 2000mW TX power)
* MPU9150 (gyroscope + accelerometer + magnetometer)
* ADS1015 (ADC/Voltmeter)
* ACS709 (current sensor)
* 4x Afro 12A ESCs (flashed with BLHeli, DampedLight enabled)
* 4x Multistar Elite 2204-2300KV motors

Home made WiFi controller :
* Taranis X9D case
* Taranis X9D gimballs
* Raspberry Pi 2
* Waveshare 3.5" touchscreen
* Alfa Network AWUS036NH (pushed to 2000mW TX power)
* MCP3208 ADC (for gimballs input and battery voltage sensing)
* 10x ON-ON switches
