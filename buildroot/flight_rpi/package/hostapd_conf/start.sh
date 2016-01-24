#!/bin/sh

#TODO : determine which of nl80211 or rtl871xdrv should be used
ifconfig wlan0 up
/usr/sbin/dnsmasq
ifconfig wlan0 192.168.32.1
exec /usr/sbin/hostapd -B /etc/hostapd/hostapd_nl80211.conf
