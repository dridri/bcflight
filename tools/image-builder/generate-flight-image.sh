#!/bin/bash

set -xe

BASEDIR=$PWD

apt install $AUTO_Y bc fdisk binfmt-support qemu-user-static wget
update-binfmts --import


mkdir -p /tmp/raspbian/bcflight/root
cd /tmp/raspbian


#rm raspbian_lite_latest
if [ ! -f /tmp/raspbian/raspbian_lite_latest.img.xz ]; then
	wget -O raspbian_lite_latest.img.xz https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2023-05-03/2023-05-03-raspios-bullseye-armhf-lite.img.xz
fi
rm -f *.img
xz -kd raspbian_lite_latest.img.xz

# This prevent device-mapper to mixup data if this script is run more than once in a row
mv raspbian_lite_latest.img $(ls raspbian_lite_latest.img | sed "s/lite_latest/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 8 | head -n 1)-lite/g")


dd if=/dev/zero bs=1M count=$(bc <<< "(1024*2 + 256 + 512) - `stat -c %s *-lite.img` / (1024 * 1024)") >> *-lite.img
SYSTEM_START_BLOCK=$(fdisk *-lite.img <<< p | grep Linux | tr -s ' ' | cut -d' ' -f2)
fdisk *-lite.img <<EOF
d
2
n
p
2
$SYSTEM_START_BLOCK
+2G
w
EOF
DATA_START_BLOCK=$(bc <<< `fdisk *-lite.img <<< p | grep Linux | tr -s ' ' | cut -d' ' -f3`+1)
fdisk *-lite.img <<EOF
n
p
3
$DATA_START_BLOCK

p
w
EOF

# LOOP=/dev/mapper/loop8
LOOP=$1
if [ -z "$LOOP" ]; then
	LOOP=$(losetup -f)
fi
losetup -P ${LOOP} *-lite.img
fsck.ext4 -y ${LOOP}p2
resize2fs ${LOOP}p2
mkfs.ext4 ${LOOP}p3
mount -o rw ${LOOP}p2 /tmp/raspbian/bcflight/root
mount -o rw ${LOOP}p1 /tmp/raspbian/bcflight/root/boot

mkdir /tmp/raspbian/bcflight/root/var2
mount -o rw ${LOOP}p3 /tmp/raspbian/bcflight/root/var2
cp -a /tmp/raspbian/bcflight/root/var/* /tmp/raspbian/bcflight/root/var2/
umount /tmp/raspbian/bcflight/root/var2
rm -rf /tmp/raspbian/bcflight/root/var/* /tmp/raspbian/bcflight/root/var2
mount -o rw ${LOOP}p3 /tmp/raspbian/bcflight/root/var
mkdir -p /tmp/raspbian/bcflight/root/var/flight
mkdir -p /tmp/raspbian/bcflight/root/var/BLACKBOX
mkdir -p /tmp/raspbian/bcflight/root/var/PHOTO
mkdir -p /tmp/raspbian/bcflight/root/var/VIDEO

# mount binds
mount --bind /dev /tmp/raspbian/bcflight/root/dev/
mount --bind /sys /tmp/raspbian/bcflight/root/sys/
mount --bind /proc /tmp/raspbian/bcflight/root/proc/
mount --bind /dev/pts /tmp/raspbian/bcflight/root/dev/pts

# ld.so.preload fix
sed -i 's/^/#CHROOT /g' /tmp/raspbian/bcflight/root/etc/ld.so.preload

# copy qemu binary
cp $(which qemu-arm-static) /tmp/raspbian/bcflight/root$(which qemu-arm-static)
#systemctl restart systemd-binfmt.service

# chroot to raspbian
chroot /tmp/raspbian/bcflight/root <<EOF_
apt update
apt remove --purge -y dbus dphys-swapfile fake-hwclock man-db
apt install -y i2c-tools spi-tools libpigpio1 libshine3 wget kmscube libcamera-apps libcamera0 libavformat58 libavutil56 libavcodec58 libgps28 gpsd hostapd dnsmasq lua5.3 python3-gps python3-serial
apt autoremove -y
apt autoclean -y
apt clean -y
EOF_

chroot /tmp/raspbian/bcflight/root <<EOF_
passwd <<EOF
bcflight
bcflight
EOF

passwd pi <<EOF
bcflight
bcflight
EOF
EOF_

chroot /tmp/raspbian/bcflight/root <<EOF_
ln -s /tmp /var/lib/dhcp
mv /etc/resolv.conf /tmp/resolv.conf
ln -s /tmp/resolv.conf /etc/resolv.conf
EOF_

cat >> /tmp/raspbian/bcflight/root/etc/bash.bashrc <<EOF
fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")
alias ro='mount -o remount,ro / ; fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")'
alias rw='mount -o remount,rw / ; fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")'
export PS1='\[\033[01;32m\]\u@\h\${fs_mode:+(\$fs_mode)}\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\\\$ '
EOF

cat /tmp/raspbian/bcflight/root/etc/fstab | tail -n1 | sed 's/-02/-03/g' | sed 's/\//\/var/g' | sed 's/defaults/defaults,comment=var/g' >> /tmp/raspbian/bcflight/root/etc/fstab
cat >> /tmp/raspbian/bcflight/root/etc/fstab <<EOF
tmpfs      /var/tmp        tmpfs   defaults,noatime,mode=0755 0 0
tmpfs      /tmp            tmpfs   defaults,noatime,mode=0755 0 0
tmpfs      /home/pi        tmpfs   defaults,noatime,mode=0777 0 0
EOF

sed -i 's/#Port/Port/g' /tmp/raspbian/bcflight/root/etc/ssh/sshd_config
sed -i 's/#AddressFamily/AddressFamily/g' /tmp/raspbian/bcflight/root/etc/ssh/sshd_config
sed -i 's/#ListenAddress/ListenAddress/g' /tmp/raspbian/bcflight/root/etc/ssh/sshd_config
sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/g' /tmp/raspbian/bcflight/root/etc/ssh/sshd_config

sed -i 's/ExecStart=/ExecStartPre=ubxtool -S 115200 -f \/dev\/ttyAMA0\nExecStart=/g' /tmp/raspbian/bcflight/root/lib/systemd/system/gpsd.service
sed -i -r -z 's/\n\[Install\]/ExecStartPost=bash -c "sleep 1 \&\& ubxtool -p CFG-RATE,100"\n\n[Install]/g' /tmp/raspbian/bcflight/root/lib/systemd/system/gpsd.service
sed -i 's/DEVICES=""/DEVICES="\/dev\/ttyAMA0"/g' /tmp/raspbian/bcflight/root/etc/default/gpsd
sed -i 's/GPSD_OPTIONS=""/GPSD_OPTIONS="-n -G -s 115200"/g' /tmp/raspbian/bcflight/root/etc/default/gpsd

cat > /tmp/raspbian/bcflight/root/etc/network/interfaces <<EOF
source /etc/network/interfaces.d/*

auto wlan0
iface wlan0 inet static
	address 192.168.32.1
	netmask 255.255.255.0
	wireless-power off
EOF

cat > /tmp/raspbian/bcflight/root/etc/dnsmasq.conf <<EOF
interface=wlan0
dhcp-range=192.168.32.50,192.168.32.150,12h
EOF

cat > /tmp/raspbian/bcflight/root/etc/hostapd/hostapd.conf <<EOF
interface=wlan0
driver=nl80211
ssid=drone
hw_mode=g
channel=6
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0 #
wpa=2
wpa_passphrase=12345678
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
EOF

cat > /tmp/raspbian/bcflight/root/lib/systemd/system/flight.service <<EOF
[Unit]
Description=Beyond-Chaos Flight Controller

[Service]
WorkingDirectory=/var/flight
ExecStart=/var/flight/flight > /var/flight/flight.log
IgnoreSIGPIPE=true
KillMode=process
Restart=always
StartLimitBurst=3
StartLimitIntervalSec=30

[Install]
WantedBy=multi-user.target
EOF

chroot /tmp/raspbian/bcflight/root <<EOF_
rm /etc/systemd/system/dhcpcd.service.d/wait.conf

# /bin/rm -f -v /etc/ssh/ssh_host_*_key*
# /usr/bin/ssh-keygen -A -v
# systemctl disable regenerate_ssh_host_keys

systemctl enable ssh
systemctl disable flight # systemctl enable flight
systemctl disable userconfig
systemctl disable fake-hwclock
/lib/systemd/systemd-sysv-install disable resize2fs_once

systemctl enable hostapd
systemctl enable dnsmasq

echo i2c-dev >> /etc/modules
EOF_


cp /tmp/raspbian/bcflight/root/usr/lib/raspberrypi-sys-mods/firstboot /tmp/raspbian/bcflight/root/usr/lib/raspberrypi-sys-mods/firstboot_bak
echo "#!/bin/bash
#set -xe
$(grep -B1000 debug_quirks2 /tmp/raspbian/bcflight/root/usr/lib/raspberrypi-sys-mods/firstboot_bak)
mount / -o remount,rw
sed -i 's|quiet|consoleblank=0 noswap ro nosplash fastboot vc4.fkms_max_refresh_rate=50000|g' /boot/cmdline.txt
sed -i 's|console=serial0,115200 ||g' /boot/cmdline.txt
sed -i -r '1 ! s/([vfatex4]+\s*defaults)(,r[ow])?(,?)/\1,ro\3/g' /etc/fstab
sed -i '1 ! s/defaults,ro,comment=var/defaults,rw/g' /etc/fstab
mount / -o remount,ro
$(grep -A1000 debug_quirks2 /tmp/raspbian/bcflight/root/usr/lib/raspberrypi-sys-mods/firstboot_bak | tail -n+2)" \
	| sed -r 's/mount\s+\/boot\s*$/mount \/boot \&\& mount \/var/g' \
	| sed -r 's|findmnt\s+/\s+|findmnt /var |g' \
	| sed -r 's/get_variables\s*\(\)\s*\{/get_variables \(\) {\n  REAL_ROOT_PART_DEV=$(findmnt \/ -o source -n)/g' \
	| sed -r 's/mount\s+\/\s+\-o remount,r([ow])/mount \/ \-o remount,r\1 \&\& mount \/var \-o remount,r\1/g' \
	| sed -r 's/mount\s+\-o remount,r([ow])\s+\/$/mount \/ \-o remount,r\1 \&\& mount \/var \-o remount,r\1/g' \
	| sed -r 's/^\s+DISKID\s*=/  mount -o remount,rw "$REAL_ROOT_PART_DEV"\n  DISKID=/g' \
	| sed -r 's/\s*mount -o remount,ro "\$BOOT_PART_DEV"/  mount -o remount,ro "\$REAL_ROOT_PART_DEV"\n  mount -o remount,ro "\$BOOT_PART_DEV"/g' \
	| sed -r 's/resize2fs/resize2fs -f/g' \
	| sed -r 's/\bparted/parted -s/g' \
	| sed -r 's/^\s*do_resize\s*$/    umount -f \/var\n    do_resize/g' \
	| sed -r ':a;N;$!ba;s/\bmount\s+\/var\s+\-o\s+remount,rw\s*\n\s*resize2fs/mount \/var -o rw\n  resize2fs/g' \
	> /tmp/raspbian/bcflight/root/usr/lib/raspberrypi-sys-mods/firstboot

# 	| sed -r 's/^reboot_pi$//g' \
# 	| sed -r 's/whiptail/#whiptail/g' \

sed -i 's/dtoverlay=vc4\-kms\-v3d/#dtoverlay=vc4-kms-v3d/g' /tmp/raspbian/bcflight/root/boot/config.txt
sed -i 's/dtparam=audio=on/dtparam=audio=off/g' /tmp/raspbian/bcflight/root/boot/config.txt

cat >> /tmp/raspbian/bcflight/root/boot/config.txt <<EOF
dtparam=i2c_arm=on,i2c_arm_baudrate=400000
# dtparam=i2c1=on,i2c1_baudrate=400000
dtparam=spi=on

dtparam=audio=off
dtoverlay=pi3-disable-bt

dtoverlay=vc4-fkms-v3d

## DShot
# Uncoment the lines below to enable DShot
#dpi_output_format=0x7017
#dpi_group=2
#dpi_mode=87
#dpi_timings=512 0 40 80 40 1 0 0 1 0 0 0 0 10000 0 9309309 6
##dpi_timings=32 0 0 0 0 16 0 0 16 0 0 0 0 10000 0 10227272 6
#enable_dpi_lcd=1
#display_default_lcd=0
#dtoverlay=dpi4,gpio_pin0=4,gpio_pin1=5,gpio_pin2=6,gpio_pin3=7 # Change pins accordingly to config.lua


## HDMI output (use HDMI0 plug if DSHot DPI hack is used (because HDMI1 and DPI seem to share their pixel clock))
framebuffer_width=1920
framebuffer_height=1080
hdmi_group=2
hdmi_mode=82

## Analog video output
# Uncoment the lines below to enable Composite output
#enable_tvout=1
#framebuffer_priority=3

start_x=1
force_turbo=1
boot_delay=1
gpu_mem=256
gpu_mem_1024=320

dtoverlay=dwc2,dr_mode=host
EOF

wget --no-check-certificate https://datasheets.raspberrypi.com/cmio/dt-blob-cam1.bin -O /tmp/raspbian/bcflight/root/boot/dt-blob.bin

if test -f "$BASEDIR/../../flight/extras/rpi/dpi4.dtbo"; then
	cp "$BASEDIR/../../flight/extras/rpi/dpi4.dtbo" /tmp/raspbian/bcflight/root/boot/overlays/dpi4.dtbo
fi

if test -f "$BASEDIR/../../flight/build/flight"; then
	cp "$BASEDIR/../../flight/build/flight" /tmp/raspbian/bcflight/root/var/flight/flight
fi

if test -f "$BASEDIR/../../flight/build/flight_unstripped"; then
	cp "$BASEDIR/../../flight/build/flight_unstripped" /tmp/raspbian/bcflight/root/var/flight/flight_unstripped
fi

if test -d "$BASEDIR/../../libhud/data"; then
	cp -r "$BASEDIR/../../libhud/data" /tmp/raspbian/bcflight/root/var/flight/data
fi

cp $BASEDIR/basic-config.lua /tmp/raspbian/bcflight/root/var/flight/config.lua


echo "Now entering chroot to let you add extra things, type 'exit' when finised"
chroot /tmp/raspbian/bcflight/root

# ----------------------------
# Clean up
# revert ld.so.preload fix
sed -i 's/^#CHROOT //g' /tmp/raspbian/bcflight/root/etc/ld.so.preload

# unmount everything
umount -l /tmp/raspbian/bcflight/root/{dev/pts,dev,sys,proc,boot,var,} || true
umount -l /tmp/raspbian/bcflight/root || true
losetup -d ${LOOP} 2>/dev/null || true

OUTFILE=$(date +"%Y-%m-%d")-raspbian-bcflight.img
mv *-lite.img $BASEDIR/$OUTFILE

chown `stat -c "%u:%g" $BASEDIR` $BASEDIR/$OUTFILE
