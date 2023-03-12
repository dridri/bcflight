#!/bin/bash

set -x

BASEDIR=$PWD

apt install kpartx qemu binfmt-support qemu-user-static
update-binfmts --import


mkdir -p /tmp/raspbian/bcflight/root
cd /tmp/raspbian


#rm raspbian_lite_latest
if [ ! -f /tmp/raspbian/raspbian_lite_latest ]; then
	wget -O raspbian_lite_latest https://downloads.raspberrypi.org/raspbian_lite_latest
fi
rm -f bcflight/*.img
unzip raspbian_lite_latest -d bcflight
cd bcflight

# This prevent device-mapper to mixup data if this script is run more than once in a row
mv *-lite.img $(ls *-lite.img | sed "s/lite/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 8 | head -n 1)-lite/g")


dd if=/dev/zero bs=1M count=256 >> *-lite.img
DATA_START_BLOCK=$(bc <<< `fdisk *-lite.img <<< p | grep Linux | tr -s ' ' | cut -d' ' -f3`+1)
fdisk *-lite.img <<EOF
n
p
3
$DATA_START_BLOCK

p
w
EOF

LOOP=$(kpartx -v -a *-lite.img | head -n1 | cut -d' ' -f3 | cut -c1- | cut -c-6)
mount -o rw /dev/mapper/${LOOP}p2 /tmp/raspbian/bcflight/root
mount -o rw /dev/mapper/${LOOP}p1 /tmp/raspbian/bcflight/root/boot
mkfs.ext4 /dev/mapper/${LOOP}p3

mkdir /tmp/raspbian/bcflight/root/var2
mount -o rw /dev/mapper/${LOOP}p3 /tmp/raspbian/bcflight/root/var2
cp -a /tmp/raspbian/bcflight/root/var/* /tmp/raspbian/bcflight/root/var2/
umount /tmp/raspbian/bcflight/root/var2
rm -rf /tmp/raspbian/bcflight/root/var/* /tmp/raspbian/bcflight/root/var2
mount -o rw /dev/mapper/${LOOP}p3 /tmp/raspbian/bcflight/root/var
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
cp $(which qemu-arm) /tmp/raspbian/bcflight/root/$(which qemu-arm)


# chroot to raspbian
chroot /tmp/raspbian/bcflight/root <<EOF_
apt update
apt remove --purge -y logrotate dbus dphys-swapfile fake-hwclock wpasupplicant man-db
apt install -y wiringpi i2c-tools spi-tools gpio-utils libshine3

passwd <<EOF
bcflight
bcflight
EOF

dphys-swapfile swapoff
dphys-swapfile uninstall
update-rc.d dphys-swapfile disable

ln -s /tmp /var/lib/dhcp
mv /etc/resolv.conf /tmp/resolv.conf
ln -s /tmp/resolv.conf /etc/resolv.conf

cat >> /etc/bash.bashrc <<EOF
fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")
alias ro='mount -o remount,ro / ; fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")'
alias rw='mount -o remount,rw / ; fs_mode=\$(mount | sed -n -e "s/^.* on \/ .*(\(r[w|o]\).*/\1/p")'
export PS1='\[\033[01;32m\]\u@\h\${fs_mode:+(\$fs_mode)}\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\\\$ '
EOF

sed -i '1 ! s/defaults/defaults,ro/g' /etc/fstab
cat /etc/fstab | tail -n1 | sed 's/-02/-03/g' | sed 's/\//\/var/g' | sed 's/,ro/,rw/g' >> /etc/fstab
cat >> /etc/fstab <<EOF
tmpfs      /var/log        tmpfs   defaults,noatime,mode=0755 0 0
tmpfs      /var/tmp        tmpfs   defaults,noatime,mode=0755 0 0
tmpfs      /tmp            tmpfs   defaults,noatime,mode=0755 0 0
tmpfs      /home/pi        tmpfs   defaults,noatime,mode=0777 0 0
EOF

sed -i 's/#Port/Port/g' /etc/ssh/sshd_config
sed -i 's/#AddressFamily/AddressFamily/g' /etc/ssh/sshd_config
sed -i 's/#ListenAddress/ListenAddress/g' /etc/ssh/sshd_config
sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/g' /etc/ssh/sshd_config

cat > /lib/systemd/system/flight.service <<EOF
[Unit]
Description=Beyond-Chaos Flight Controller

[Service]
WorkingDirectory=/var/flight
ExecStart=/var/flight/flight > /var/flight/flight.log
IgnoreSIGPIPE=true
KillMode=process

[Install]
WantedBy=multi-user.target
EOF

rm /etc/systemd/system/dhcpcd.service.d/wait.conf

/bin/rm -f -v /etc/ssh/ssh_host_*_key*
/usr/bin/ssh-keygen -A -v

systemctl enable ssh
systemctl enable flight
systemctl disable fake-hwclock
systemctl disable regenerate_ssh_host_keys
/lib/systemd/systemd-sysv-install disable resize2fs_once

sed -i 's|quiet||g' /boot/cmdline.txt
sed -i 's|init=/usr/lib/raspi-config/init_resize\.sh|consoleblank=0 noswap ro nosplash fastboot vc4.fkms_max_refresh_rate=50000|g' /boot/cmdline.txt
cat >> /boot/config.txt <<EOF
dtparam=i2c_arm=on,i2c_arm_baudrate=400000
dtparam=i2c1=on,i2c1_baudrate=400000
dtparam=spi=on

dtparam=audio=off
dtoverlay=pi3-disable-bt

dpi_output_format=0x1C17
dpi_group=2
dpi_mode=87
dpi_timings=512 0 40 80 40 1 0 0 1 0 0 0 0 10000 0 9309309 6
dtoverlay=dpi4,gpio_pin0=4,gpio_pin1=5,gpio_pin2=6,gpio_pin3=7
dtoverlay=vc4-fkms-v3d

start_x=1
force_turbo=1
boot_delay=1
gpu_mem=256
gpu_mem_1024=320
EOF

EOF_


# echo "Now entering chroot, type 'exit' when finised"
# chroot /tmp/raspbian/bcflight/root

# ----------------------------
# Clean up
# revert ld.so.preload fix
sed -i 's/^#CHROOT //g' /tmp/raspbian/bcflight/root/etc/ld.so.preload

# unmount everything
umount /tmp/raspbian/bcflight/root/{dev/pts,dev,sys,proc,boot,var,}
kpartx -dv /dev/$LOOP


OUTFILE=$(date +"%Y-%m-%d")-raspbian-bcflight.img
mv *-lite.img $BASEDIR/$OUTFILE
chown `stat -c "%u:%g" $BASEDIR` $BASEDIR/$OUTFILE
