#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "nl80211.h"

typedef struct nl80211_state {
	struct nl_sock* nl_sock;
	int nl80211_id;
	char ifr[32];
	uint8_t hwaddr[6];
	int fd;
	int devidx;
	int phyidx;
} nl80211_state;

static int ieee80211_channel_to_frequency( int chan, enum nl80211_band band );
static int phymactoindex( const char* hwaddr );
static struct nl_msg* msg_init( struct nl80211_state* state, int cmd );
static int msg_send( nl80211_state* state, struct nl_msg* msg );
static int interface_setifflags( int s, const char* vname, int32_t value );
static int interface_set_monitor( struct nl80211_state* state );
static int interface_set_frequency( struct nl80211_state* state, int32_t freq, int32_t ht_bandwidth );
static int interface_set_txpower( struct nl80211_state* state, int32_t power_mbm );
static int interface_set_bitrate( struct nl80211_state* state, uint32_t is_ht, uint32_t rate );


static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,  void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

int valid_handler(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

int rawwifi_setup_interface( const char* name, uint32_t channel, uint32_t txpower, uint32_t bHT, uint32_t bitrate )
{
	nl80211_state* state = (nl80211_state*)malloc( sizeof( nl80211_state ) );
	strncpy( state->ifr, name, sizeof(state->ifr) );
	state->fd = socket( AF_INET, SOCK_DGRAM, 0 );

	struct ifreq ifr;
	strcpy( ifr.ifr_name, name );
	ioctl( state->fd, SIOCGIFHWADDR, &ifr );
	memcpy( state->hwaddr, ifr.ifr_hwaddr.sa_data, 6 );
	char hwaddr[64] = "";
	sprintf( hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x", state->hwaddr[0], state->hwaddr[1], state->hwaddr[2], state->hwaddr[3], state->hwaddr[4], state->hwaddr[5] );

	state->devidx = if_nametoindex( name );
	state->phyidx = phymactoindex( hwaddr );

	if ( state->phyidx < 0 ) {
		printf( "Error : interface '%s' not found !\n", name );
		return -1;
	}

	printf( "Interface %s :\n"
			"    HWaddr = %s\n"
			"    devidx = %d\n"
			"    phyidx = %d\n", name, hwaddr, state->devidx, state->phyidx );

	state->nl_sock = nl_socket_alloc();
	if ( !state->nl_sock ) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}
	if ( genl_connect( state->nl_sock ) ) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		return -ENOLINK;
	}
	nl_socket_set_buffer_size( state->nl_sock, 8192, 8192 );
	state->nl80211_id = genl_ctrl_resolve( state->nl_sock, "nl80211" );
	if ( state->nl80211_id < 0 ) {
		fprintf(stderr, "nl80211 not found.\n");
		return -ENOENT;
	}

	printf( "Stopping interface...\n" );
	interface_setifflags( state->fd, name, -IFF_UP );
	printf( "Setting monitor mode...\n" );
	interface_set_monitor( state );
	printf( "Starting interface...\n" );
	interface_setifflags( state->fd, name, IFF_UP );
	if ( channel != 0 ) {
		enum nl80211_band band = ( channel <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ );
		int freq = ieee80211_channel_to_frequency( channel, band );
		printf( "Setting channel to %d (%dMHz)...\n", channel, freq );
		interface_set_frequency( state, freq, 0 );
	}
	if ( txpower != 0 ) {
		printf( "Setting TX power to %d000mBm...\n", txpower );
		interface_set_txpower( state, txpower * 1000 );
	}
	if ( bitrate != 0 ) {
		if ( bHT ) {
			printf( "Setting bitrate to %d HT-MCS\n", bitrate );
		} else {
			printf( "Setting bitrate to %d Mbps\n", bitrate );
		}
		interface_set_bitrate( state, bHT, bitrate );
	}

	close( state->fd );
	return 0;
}


struct nl_msg* msg_init( nl80211_state* state, int cmd )
{
	struct nl_msg* msg = nlmsg_alloc();
	genlmsg_put( msg, 0, 0, state->nl80211_id, 0, 0, cmd, 0 );

	nla_put_u32( msg, NL80211_ATTR_IFINDEX, state->devidx );
	nla_put_u32( msg, NL80211_ATTR_WIPHY, state->phyidx );

	return msg;
}


int msg_send( nl80211_state* state, struct nl_msg* msg )
{
	struct nl_cb* cb = nl_cb_alloc( NL_CB_DEFAULT );
	struct nl_cb* s_cb = nl_cb_alloc( NL_CB_DEFAULT );
	nl_socket_set_cb( state->nl_sock, s_cb );
	int err = nl_send_auto_complete(state->nl_sock, msg);
	if ( err < 0 ) {
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


static int interface_setifflags( int s, const char* vname, int32_t value )
{
	struct ifreq ifreq;
	(void) strncpy( ifreq.ifr_name, vname, sizeof(ifreq.ifr_name) );

	if ( ioctl( s, SIOCGIFFLAGS, &ifreq ) < 0 ) {
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
	if ( ioctl( s, SIOCSIFFLAGS, &ifreq ) < 0 ) {
		return errno;
	}
	return 0;
}


static int interface_set_monitor( struct nl80211_state* state )
{
	struct nl_msg* msg = msg_init( state, NL80211_CMD_SET_INTERFACE );
	struct nl_msg* flags = nlmsg_alloc();

	nla_put_u32( msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR );
	nla_put( flags, NL80211_MNTR_FLAG_FCSFAIL, 0, NULL );
	nla_put( flags, NL80211_MNTR_FLAG_OTHER_BSS, 0, NULL );
	nla_put_nested( msg, NL80211_ATTR_MNTR_FLAGS, flags );

	nlmsg_free( flags );
	msg_send( state, msg );
	return 0;
}


static int interface_set_frequency( struct nl80211_state* state, int32_t freq, int32_t ht_bandwidth )
{
	struct nl_msg* msg = msg_init( state, NL80211_CMD_SET_WIPHY );
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

	msg_send( state, msg );
	return 0;
}


static int interface_set_txpower( struct nl80211_state* state, int32_t power_mbm )
{
	struct nl_msg* msg = msg_init( state, NL80211_CMD_SET_WIPHY );

	nla_put_u32( msg, NL80211_ATTR_WIPHY_TX_POWER_SETTING, NL80211_TX_POWER_FIXED );
	nla_put_u32( msg, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, power_mbm );

	msg_send( state, msg );
	return 0;
}


static int interface_set_bitrate( struct nl80211_state* state, uint32_t is_ht, uint32_t rate )
{
	struct nl_msg* msg = msg_init( state, NL80211_CMD_SET_WIPHY );
	struct nlattr* nl_rates = nla_nest_start( msg, NL80211_ATTR_TX_RATES );

	struct nlattr* nl_band = nla_nest_start(msg, NL80211_BAND_2GHZ);
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
	msg_send( state, msg );
	return 0;
}


static int ieee80211_channel_to_frequency( int chan, enum nl80211_band band )
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


int phymactoindex( const char* hwaddr )
{
	char path[256] = "";
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
					if ( !strcmp( buf, hwaddr ) ) {
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
