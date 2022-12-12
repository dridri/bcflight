## Touchscreen rotation

* default : `echo 'ENV{LIBINPUT_CALIBRATION_MATRIX}="1 0 0 0 1 0"' > /etc/udev/rules.d/libinput.rules`
* 90deg clockwise : `echo 'ENV{LIBINPUT_CALIBRATION_MATRIX}="0 -1 1 1 0 0"' > /etc/udev/rules.d/libinput.rules`
* 180deg clockwise : `echo 'ENV{LIBINPUT_CALIBRATION_MATRIX}="-1 0 1 0 -1 1"' > /etc/udev/rules.d/libinput.rules`
* 270deg clockwise : `echo 'ENV{LIBINPUT_CALIBRATION_MATRIX}="0 1 0 -1 0 1"' > /etc/udev/rules.d/libinput.rules`

Then reboot
