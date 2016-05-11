# BeyondChaos Flight
This project aims to create open-source flight controller and ground controller for Linux based drones.

At this time, you won't be able to fly a drone using this project, as the controller is not finished yet.

## flight
Current status : flying - beta

The 'flight' subfolder contains the flight controller itself, currently supporting only Raspberry Pi board, but can be easily extended to any kind of board (including non-Linux ones, but it is not the purpose of this project).

## controller_rpi
Current status : in progress

This subfolder contains a ground controller specifically designed for Raspberry Pi.

## librawwifi
Current status : in progress

This is a fork of wifibroadcast from befinitiv, it allows to directly use WiFi in analog-like transmission.

Alternatively, hostapd is used on the drone and wpa_supplicant on the controller to allow socket communication.

## tools
Current status : ok

Generation scripts for easier building

## buildroot
Current status : in progress

Contains buildroot configuration for both flight and ground targets.


# Current test machine
(videos to come shortly)

Beyond Chaos Racer 280 :
![Image of drone](https://dl.dropboxusercontent.com/u/81758777/drich-drone-vtail-small.jpg)
* Lynxmotion/DiaLFonZo mini V-Tail frame
* Raspberry Pi A+
* Raspberry Pi official 5MP camera
* Alfa Network AWUS036NH
* MPU9150 (gyroscope + accelerometer + magnetometer)
* ADS1015 (ADC/Voltmeter)
* ACS709 (current sensor)
* 4x Afro 12A ESC
* 4x Lynxmotion 1804-2400KV motor

First home made WiFi controller :
![Image of controller](https://dl.dropboxusercontent.com/u/81758777/drone_rc_controller_small.jpg)
