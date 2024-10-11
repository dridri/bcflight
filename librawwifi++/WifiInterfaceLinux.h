#pragma once

#include "WifiInterface.h"
#include "nl80211.h"

struct nl_msg;

namespace rawwifi {

class WifiInterfaceLinux : public WifiInterface {
public:
	WifiInterfaceLinux( const std::string name, uint32_t channel, uint32_t txpower, uint32_t bHT, uint32_t bitrate );
	virtual ~WifiInterfaceLinux();

protected:
	typedef struct nl80211_state {
		struct nl_sock* nl_sock;
		int32_t nl80211_id;
		char ifr[32];
		uint8_t hwaddr[6];
		int32_t fd;
		int32_t devidx;
		int32_t phyidx;
	} nl80211_state;

	int32_t phyMacToIndex( const std::string& hwaddr );
	int32_t setFlags( int32_t fd, const std::string& vname, int32_t value );
	int32_t setMonitor( nl80211_state* state );
	int32_t setFrequency( nl80211_state* state, int32_t freq, int32_t ht_bandwidth );
	int32_t setTxPower( nl80211_state* state, int32_t power_mbm );
	int32_t setBitrate( nl80211_state* state, uint32_t is_ht, uint32_t rate );

	struct nl_msg* nlMsgInit( nl80211_state* state, int cmd );
	int32_t nlMsgSend( nl80211_state* state, struct nl_msg* msg );

	int32_t ieee80211_channel_to_frequency( int chan, nl80211_band band );
};

} // namespace rawwifi
