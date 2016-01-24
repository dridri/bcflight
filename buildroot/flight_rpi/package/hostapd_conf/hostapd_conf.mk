################################################################################
#
# hostapd_conf
#
################################################################################

HOSTAPD_CONF_VERSION = 1.0
HOSTAPD_CONF_SITE =
HOSTAPD_CONF_SOURCE =
HOSTAPD_CONF_SUBDIR =
HOSTAPD_CONF_CONFIG = $(HOSTAPD_CONF_DIR)/.config
HOSTAPD_CONF_DEPENDENCIES =
HOSTAPD_CONF_CFLAGS = $(TARGET_CFLAGS)
HOSTAPD_CONF_LICENSE =
HOSTAPD_CONF_LICENSE_FILES =
HOSTAPD_CONF_CONFIG_SET =
HOSTAPD_CONF_CONFIG_ENABLE =
HOSTAPD_CONF_CONFIG_DISABLE =


define HOSTAPD_CONF_INSTALL_TARGET_CMDS
	@mkdir -p $(TARGET_DIR)/etc/hostapd
	$(INSTALL) -D -m 0644 package/hostapd_conf/hostapd_nl80211.conf $(TARGET_DIR)/etc/hostapd/
	$(INSTALL) -D -m 0644 package/hostapd_conf/hostapd_rtl871xdrv.conf $(TARGET_DIR)/etc/hostapd/
	$(INSTALL) -D -m 0755 package/hostapd_conf/start.sh $(TARGET_DIR)/etc/hostapd/
	$(INSTALL) -D -m 0644 package/hostapd_conf/hostapd.service $(TARGET_DIR)/etc/systemd/system/
	ln -sf $(TARGET_DIR)/etc/systemd/system/hostapd.service $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants/hostapd.service
	$(INSTALL) -D -m 0644 package/hostapd_conf/dnsmasq.conf $(TARGET_DIR)/etc/dnsmasq.conf
endef

$(eval $(generic-package))
