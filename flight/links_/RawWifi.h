#ifndef RAWWIFI_LINK_H
#define RAWWIFI_LINK_H

#include "Link.h"
#include "Config.h"
#include <Thread.h>
#include <condition_variable>

namespace rawwifi {
	class RawWifi;
	class WifiInterface;
};

LUA_CLASS class RawWifi : public Link, protected Thread
{
public:
	LUA_EXPORT RawWifi();
	virtual ~RawWifi();

	virtual int Connect();
	virtual int setBlocking( bool blocking );
	virtual void setRetriesCount( int retries );
	virtual int retriesCount() const;
	virtual int32_t Channel();
	virtual int32_t Frequency();
	virtual int32_t RxQuality();
	virtual int32_t RxLevel();

	virtual SyncReturn Read( void* buf, uint32_t len, int32_t timeout );
	virtual SyncReturn Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );

	virtual SyncReturn WriteAck( const void* buf, uint32_t len ) { return Write( buf, len, false, -1 ); }

	virtual string name() const { return "RawWifi"; }
	virtual LuaValue infos() const { return LuaValue(); }

protected:
	LUA_PROPERTY("device") std::string mDevice;
	LUA_PROPERTY("output_port") uint8_t mOutputPort;
	LUA_PROPERTY("input_port") uint8_t mInputPort;
	LUA_PROPERTY("channel") uint32_t mChannel;
	LUA_PROPERTY("blocking") bool mBlocking;

	rawwifi::RawWifi* mRawWifi;
	rawwifi::WifiInterface* mWifiInterface;

	std::list< std::vector<uint8_t> > mPacketsQueue;
	std::mutex mPacketsQueueMutex;
	std::condition_variable mPacketsQueueCond;

	virtual bool run();
};

#endif // RAWWIFI_LINK_H
