#include <unistd.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "WifiInterfaceLinux.h"
#include "Debug.h"

using namespace rawwifi;

WifiInterfaceLinux::WifiInterfaceLinux( const std::string name, uint32_t channel, uint32_t txpower, uint32_t bHT, uint32_t bitrate )
	: WifiInterface( name, channel, txpower, bHT, bitrate )
{
	fDebug( name, channel, txpower, bHT, bitrate );
	int ret = 0;

	nl80211_state* state = (nl80211_state*)malloc( sizeof( nl80211_state ) );
	strncpy( state->ifr, name.c_str(), sizeof(state->ifr) );
	state->fd = socket( AF_INET, SOCK_DGRAM, 0 );

	struct ifreq ifr;
	strcpy( ifr.ifr_name, name.c_str() );
	ioctl( state->fd, SIOCGIFHWADDR, &ifr );
	memcpy( state->hwaddr, ifr.ifr_hwaddr.sa_data, 6 );
	char hwaddr[64] = "";
	sprintf( hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x", state->hwaddr[0], state->hwaddr[1], state->hwaddr[2], state->hwaddr[3], state->hwaddr[4], state->hwaddr[5] );

	state->devidx = if_nametoindex( name.c_str() );
	state->phyidx = phyMacToIndex( hwaddr );

	if ( state->phyidx < 0 ) {
		throw std::runtime_error( "Error : interface '" + name + "' not found !" );
	}

	gDebug() << "Interface " << name << ":\n"
			<< "	HWaddr = " << hwaddr << "\n"
			<< "	devidx = " << state->devidx << "\n"
			<< "	phyidx = " << state->phyidx;

	state->nl_sock = nl_socket_alloc();
	if ( !state->nl_sock ) {
		throw std::runtime_error( "Error : Failed to allocate netlink socket." );
	}
	if ( genl_connect( state->nl_sock ) ) {
		throw std::runtime_error( "Error : Failed to connect to generic netlink." );
	}
	nl_socket_set_buffer_size( state->nl_sock, 8192, 8192 );
	state->nl80211_id = genl_ctrl_resolve( state->nl_sock, "nl80211" );
	if ( state->nl80211_id < 0 ) {
		throw std::runtime_error( "Error : nl80211 not found for interface '" + name + "'." );
	}

	gDebug() << "Stopping interface...";
	setFlags( state->fd, name, -IFF_UP );
	gDebug() << "Setting monitor mode...";
	setMonitor( state );
	gDebug() << "Starting interface...";
	setFlags( state->fd, name, IFF_UP );
	if ( channel != 0 ) {
		enum nl80211_band band = ( channel <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ );
		int freq = ieee80211_channel_to_frequency( channel, band );
		gDebug() << "Setting channel to " << channel << " (" << freq << "MHz)...";
		if ( ( ret = setFrequency( state, freq, 0 ) < 0 ) ) {
			gError() << "Error : Failed to set frequency : " << ret;
		}
	}
	if ( txpower != 0 ) {
		gDebug() << "Setting TX power to " << txpower << "000mBm...";
		if ( ( ret = setTxPower( state, txpower * 1000 ) < 0 ) ) {
			gWarning() << "Error : Failed to set TX power : " << ret;
		}
	}
	if ( bitrate != 0 ) {
		if ( bHT ) {
			gDebug() << "Setting bitrate to " << bitrate << " HT-MCS with " << bHT << "MHz bandwidth...";
		} else {
			gDebug() << "Setting bitrate to " << bitrate << " Mbps...";
		}
		if ( ( ret = setBitrate( state, bHT, bitrate ) < 0 ) ) {
			gError() << "Error : Failed to set bitrate : " << ret;
		}
	}

	close( state->fd );
}


WifiInterfaceLinux::~WifiInterfaceLinux()
{
}


int32_t WifiInterfaceLinux::setMonitor( WifiInterfaceLinux::nl80211_state* state )
{
	struct nl_msg* msg = nlMsgInit( state, NL80211_CMD_SET_INTERFACE );
	struct nl_msg* flags = nlmsg_alloc();

	nla_put_u32( msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR );
	nla_put( flags, NL80211_MNTR_FLAG_FCSFAIL, 0, NULL );
	nla_put( flags, NL80211_MNTR_FLAG_OTHER_BSS, 0, NULL );
	nla_put_nested( msg, NL80211_ATTR_MNTR_FLAGS, flags );

	nlmsg_free( flags );
	nlMsgSend( state, msg );
	return 0;
}


int32_t WifiInterfaceLinux::setFrequency( WifiInterfaceLinux::nl80211_state* state, int32_t freq, int32_t ht_bandwidth )
{
	struct nl_msg* msg = nlMsgInit( state, NL80211_CMD_SET_WIPHY );
	nla_put_u32( msg, NL80211_ATTR_WIPHY_FREQ, freq );

	if ( ht_bandwidth == 0 ) {
		nla_put_u32( msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_NO_HT );
	} else if ( ht_bandwidth == 20 ) {
		nla_put_u32( msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT20 );
	} else if ( ht_bandwidth == -40 ) {
		nla_put_u32( msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT40MINUS );
	} else if ( ht_bandwidth == 40 ) {
		nla_put_u32( msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT40PLUS );
	}

	return nlMsgSend( state, msg );
}


int32_t WifiInterfaceLinux::setTxPower( nl80211_state* state, int32_t power_mbm )
{
	struct nl_msg* msg = nlMsgInit( state, NL80211_CMD_SET_WIPHY );

	nla_put_u32( msg, NL80211_ATTR_WIPHY_TX_POWER_SETTING, NL80211_TX_POWER_FIXED );
	nla_put_u32( msg, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, power_mbm );

	return nlMsgSend( state, msg );
}


int32_t WifiInterfaceLinux::setBitrate( nl80211_state* state, uint32_t is_ht, uint32_t rate )
{
	struct nl_msg* msg = nlMsgInit( state, NL80211_CMD_SET_WIPHY );
	struct nlattr* nl_rates = nla_nest_start( msg, NL80211_ATTR_TX_RATES );

	struct nlattr* nl_band = nla_nest_start( msg, NL80211_BAND_2GHZ );
// 	nl_band = nla_nest_start(msg, NL80211_BAND_5GHZ);
	if ( is_ht ) {
		nla_put( msg, NL80211_TXRATE_HT, 1, &rate );
	} else {
		nla_put( msg, NL80211_TXRATE_LEGACY, 1, &rate );
	}
// 	nla_put_u8( msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_SGI );
	nla_put_u8( msg, NL80211_TXRATE_GI, NL80211_TXRATE_FORCE_LGI ); // Reduce data overlapping with other stations
	nla_nest_end( msg, nl_band );

	nla_nest_end( msg, nl_rates );
	return nlMsgSend( state, msg );
}


int32_t WifiInterfaceLinux::ieee80211_channel_to_frequency( int chan, nl80211_band band )
{
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
	default:
		;
	}
	return 0; /* not supported */
}


int32_t WifiInterfaceLinux::setFlags( int32_t fd, const std::string& vname, int32_t value )
{
	struct ifreq ifreq;
	(void) strncpy( ifreq.ifr_name, vname.c_str(), sizeof(ifreq.ifr_name) );

	if ( ioctl( fd, SIOCGIFFLAGS, &ifreq ) < 0 ) {
		return errno;
	}
 	uint32_t flags = ifreq.ifr_flags;

	if ( value < 0 ) {
		value = -value;
		flags &= ~value;
	} else {
		flags |= value;
	}

	ifreq.ifr_flags = flags;
	if ( ioctl( fd, SIOCSIFFLAGS, &ifreq ) < 0 ) {
		return errno;
	}
	return 0;
}


struct nl_msg* WifiInterfaceLinux::nlMsgInit( nl80211_state* state, int cmd )
{
	struct nl_msg* msg = nlmsg_alloc();
	genlmsg_put( msg, 0, 0, state->nl80211_id, 0, 0, cmd, 0 );

	nla_put_u32( msg, NL80211_ATTR_IFINDEX, state->devidx );
	nla_put_u32( msg, NL80211_ATTR_WIPHY, state->phyidx );

	return msg;
}


int32_t WifiInterfaceLinux::nlMsgSend( nl80211_state* state, struct nl_msg* msg )
{
	static const auto error_handler = [](struct sockaddr_nl* nla, struct nlmsgerr* err, void* arg) {
		int* ret = reinterpret_cast<int*>(arg);
		*ret = err->error;
		return (int)NL_STOP;
	};

	static const auto finish_handler = [](struct nl_msg* msg, void* arg) {
		int* ret = reinterpret_cast<int*>(arg);
		*ret = 0;
		return (int)NL_SKIP;
	};

	static const auto ack_handler = [](struct nl_msg* msg, void* arg) {
		int* ret = reinterpret_cast<int*>(arg);
		*ret = 0;
		return (int)NL_STOP;
	};

	static const auto valid_handler = [](struct nl_msg* msg, void* arg) {
		return (int)NL_OK;
	};

	struct nl_cb* cb = nl_cb_alloc( NL_CB_DEFAULT );
	struct nl_cb* s_cb = nl_cb_alloc( NL_CB_DEFAULT );
	nl_socket_set_cb( state->nl_sock, s_cb );
	int err = nl_send_auto_complete(state->nl_sock, msg);
	if ( err < 0 ) {
		nlmsg_free(msg);
		return err;
	}

	err = 1;
	nl_cb_err( cb, NL_CB_CUSTOM, error_handler, &err );
	nl_cb_set( cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err );
	nl_cb_set( cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err );
	nl_cb_set( cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL );

	while ( err > 0 ) {
		nl_recvmsgs( state->nl_sock, cb );
	}

	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	return err;
}


int32_t WifiInterfaceLinux::phyMacToIndex( const std::string& hwaddr )
{
	char path[1024] = "";
	char buf[64] = "";
	struct dirent* ent;
	DIR* dir = opendir( "/sys/class/ieee80211" );

	while ( dir && ( ent = readdir( dir ) ) != NULL ) {
		if ( ent->d_name[0] != '.' ) {
			sprintf( path, "/sys/class/ieee80211/%s/macaddress", ent->d_name );
			int fd = open( path, O_RDONLY );
			if ( fd > 0 ) {
				read( fd, buf, sizeof(buf) );
				close( fd );
				if ( strlen(buf) > 0 ) {
					buf[strlen(buf)-1] = 0;
					if ( !strcmp( buf, hwaddr.c_str() ) ) {
						sprintf( path, "/sys/class/ieee80211/%s/index", ent->d_name );
						fd = open( path, O_RDONLY );
						if ( fd > 0 ) {
							read( fd, buf, sizeof(buf) );
							close( fd );
							closedir( dir );
							return atoi( buf );
						}
					}
				}
			}
		}
	}
	if ( dir ) {
		closedir( dir );
	}
	return -1;
}
