# BeyondChaos Flight
This project aims to create open-source flight controller and ground controller for Linux based drones.

At this time, you won't be able to fly a drone using this project, as the controller is not finished yet.

## flight
Current status : working - beta

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
