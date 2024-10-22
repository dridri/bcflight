#pragma once

#include <stdint.h>
#include <pcap.h>

#include <string>

namespace rawwifi {

class PcapHandler {
public:
	PcapHandler( const std::string& device, uint8_t port, bool blocking, int32_t read_timeout_ms = -1 );
	~PcapHandler();

	void CompileFilter();

	pcap_t* getPcap() const { return mPcap; }
	uint8_t getPort() const { return mPort; }
	bool getBlocking() const { return mBlocking; }
	uint32_t getHeaderLength() const { return mHeaderLength; }
protected:
	pcap_t* mPcap;
	std::string mDevice;
	uint8_t mPort;
	bool mBlocking;
	uint32_t mHeaderLength;
};

} // namespace rawwifi
