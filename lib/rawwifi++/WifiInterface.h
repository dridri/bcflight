#pragma once

#include <stdint.h>

#include <string>

namespace rawwifi {

class WifiInterface {
public:
	WifiInterface( const std::string name, uint32_t channel, uint32_t txpower, uint32_t bHT, uint32_t bitrate ) {}
	virtual ~WifiInterface() {}

protected:
	std::string mName;
	uint32_t mChannel;
	uint32_t mTxpower;
	uint32_t mBHT;
	uint32_t mBitrate;
};

} // namespace rawwifi
