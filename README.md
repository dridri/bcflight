Open-source Linux-based Raspberry drone and ground controller.

## Features
 * Supports Raspberry Pi 1/2/3/4/zero boards
 * Low CPU (<10% on Raspberry Pi 4) and RAM (~100MB) usage
 * Sensors and stabilizer loop-time up to 8kHz (on rPi4)
 * Up to 8 motors with customizable configuration matrix
 * DShot (150 & 300), OneShot125 and standard PWM motor protocols
 * Supports SPI and I²C [sensors](#supported-sensors)
 * LUA-based configuration, with fully configurable event-handling and user code execution
 * Homemade communication protocol over Wifi & FSK/LoRa radio, also supports S-BUS with limited functionnality
 * <4ms controls latency
 * <=50ms video latency (over composite output to 5GHz VTX module)
 * Live camera view over HDMI / Composite output with On-Screen Display (showing telemetry, battery status, fly speed, acceleration...)
 * Multiple camera recording in MKV file format
 * Supports [Gyroflow](https://github.com/gyroflow/gyroflow) GCSV output
 
## Supported sensors
#### IMUs
 * InvenSense ICM-42605
 * InvenSense ICM-20608
 * InvenSense MPU-9250
 * InvenSense MPU-9150
 * InvenSense MPU-6050
 * STMicroelectronics L3GD20H
 * STMicroelectronics LSM303
#### Barometers / Altimeters
 * Bosch BMP180
 * Bosch BMP280
#### Distance sensors
 * HC-SR04
#### ADC
 * Texas Instruments ADS1015
 * Texas Instruments ADS1115

## Supported communication systems
 * standard TCP/UDP/IP over ethernet / wifi
 * raw wifi (based on [wifibroadcast](https://github.com/rodizio1/EZ-WifiBroadcast))
 * Nordic Semiconductor nRF24L01
 * Semtech SX1276/77/78/79 FSK/LoRa / RFM95W
 * SBUS (limited functionnality)

## Hardware
Any form-factors of Raspberry Pi can be used, connecting sensors and peripherals using GPIO header and other dedicated connectors.
For smaller size and weight it's recommended to use a Compute Module 4 with a custom carrier board like this one : (can be found in [/electronics]) :
<img width="512px" style="max-width: 100%" alt="Raspberry Pi Compute Module 4 carrier board schematics" src="https://raw.githubusercontent.com/dridri/bcflight/master/misc/cm4_schematics.png"/>
<img width="512px" style="max-width: 100%" alt="Raspberry Pi Compute Module 4 carrier board front photo" src="https://raw.githubusercontent.com/dridri/bcflight/master/misc/cm4-fc-front.png"/>
<img width="512px" style="max-width: 100%" alt="Raspberry Pi Compute Module 4 carrier board back photo" src="https://raw.githubusercontent.com/dridri/bcflight/master/misc/cm4-fc-back.png"/>
<img width="512px" style="max-width: 100%" alt="Raspberry Pi Compute Module 4 carrier board back photo" src="https://raw.githubusercontent.com/dridri/bcflight/master/misc/cm4-on-drone.png"/>
