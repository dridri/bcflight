#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mutex>
#include "Board.h"
#include "DShotDriver.h"
#include "Debug.h"
#include "video/DRM.h"

// TODO : ifdef PI_VARIANT = 4
#include <xf86drm.h>
#include <xf86drmMode.h>

extern std::mutex __global_rpi_drm_mutex;
DShotDriver* DShotDriver::sInstance = nullptr;
uint8_t DShotDriver::sDPIMode = 7;
uint8_t DShotDriver::sDPIPinMap[8][28][2] = {
	{},	// invalid
	{},	// unused
	{	// mode 2, RGB565
		{}, {}, {}, {}, // pins 0,1,2,3 unused
		{ 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{ 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{ 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
	{	// mode 3, RGB565
		{}, {}, {}, {}, // pins 0,1,2,3 unused
		{ 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{}, {}, {},
		{ 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{}, {},
		{ 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
	{	// mode 4, RGB565
		{}, {}, {}, {}, {}, // pins 0,1,2,3,4 unused
		{ 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{}, {},
		{ 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{}, {}, {},
		{ 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
	{	// mode 5, RGB666
		{}, {}, {}, {}, // pins 0,1,2,3 unused
		{ 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{ 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{ 2, 2 }, { 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
	{	// mode 6, RGB666
		{}, {}, {}, {}, // pins 0,1,2,3 unused
		{ 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{}, {},
		{ 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{}, {},
		{ 2, 2 }, { 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
	{	// mode 7, RGB888
		{}, {}, {}, {}, // pins 0,1,2,3 unused
		{ 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
		{ 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
		{ 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }
	},
};


DShotDriver* DShotDriver::instance()
{
	if ( !sInstance ) {
		sInstance = new DShotDriver();
	}
	return sInstance;
}



DShotDriver::DShotDriver()
{
	int fd = DRM::drmFd();

	// Step 2 : find a suitable CRTC/Connector
	uint32_t crtc_id = 0xffffffff; //87;
	uint32_t connector_id = 0xffffffff; //89;
	drmModeRes* modes = drmModeGetResources(fd);
	for ( int32_t i = 0; i < modes->count_crtcs and crtc_id == 0xffffffff; i++ ) {
		drmModeCrtcPtr crtc = drmModeGetCrtc(fd, modes->crtcs[i]);
		if ( !strcmp( crtc->mode.name, "FIXED_MODE" ) and ( crtc->width == 32 or crtc->width == 512 or crtc->width == 1024 or crtc->width == 3072 ) ) {
			crtc_id = crtc->crtc_id;
		}
		drmModeFreeCrtc(crtc);
	}
	for ( int32_t i = 0; i < modes->count_connectors and connector_id == 0xffffffff; i++ ) {
		drmModeConnectorPtr connector = drmModeGetConnector(fd, modes->connectors[i]);
		drmModeEncoderPtr encoder = drmModeGetEncoder(fd, connector->encoders[0]);
		if ( connector->connection == DRM_MODE_CONNECTED and ( connector->connector_type == DRM_MODE_CONNECTOR_DPI or connector->connector_type == DRM_MODE_CONNECTOR_DSI ) and encoder->crtc_id == crtc_id ) {
			connector_id = connector->connector_id;
		}
		drmModeFreeEncoder(encoder);
		drmModeFreeConnector(connector);
	}
	drmModeFreeResources(modes);

	gDebug() << "CRTC : " << crtc_id;
	gDebug() << "Connector : " << connector_id;
	if ( crtc_id == 0xffffffff or connector_id == 0xffffffff ) {
		gError() << "No DPI CRTC/Connector found";
		exit(3);
	}


	// Step 3 : Create and allocate buffer, then attach it to CRTC
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;
	uint32_t fb_id;
	drmModeCrtcPtr crtc_info = drmModeGetCrtc( fd, crtc_id );

	memset( &creq, 0, sizeof(struct drm_mode_create_dumb) );
	creq.width = crtc_info->mode.hdisplay;
	creq.height = crtc_info->mode.vdisplay;
	creq.bpp = 24;
	gDebug() << creq.width << " x " << creq.height;

	if ( drmIoctl( fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq ) < 0 ) {
		gError() << "drmIoctl DRM_IOCTL_MODE_CREATE_DUMB failed : " << strerror(errno);
		exit(3);
	}

	gDebug() << "drmModeAddFB(" << fd << ", " << creq.width << ", " << creq.height << ", " << 24 << ", " << 24 << ", " << creq.pitch << ", " << creq.handle << ", &fb_id)";
	if ( drmModeAddFB(fd, creq.width, creq.height, 24, 24, creq.pitch, creq.handle, &fb_id) ) {
		gError() << "drmModeAddFB failed : " << strerror(errno);
		exit(3);
	}
	gDebug() << "fb_id: " << fb_id;
	gDebug() << "pitch: " << creq.pitch;

	memset( &mreq, 0, sizeof(struct drm_mode_map_dumb) );
	mreq.handle = creq.handle;

	if ( drmIoctl( fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq ) ) {
		gError() << "drmIoctl DRM_IOCTL_MODE_MAP_DUMB failed : " << strerror(errno);
		exit(3);
	}

	if ( drmModeSetCrtc( fd, crtc_id, fb_id, 0, 0, &connector_id, 1, &crtc_info->mode ) ) {
		gError() << "drmModeSetCrtc() failed : " << strerror(errno);
		exit(3);
	}
	drmModeFreeCrtc(crtc_info);

	// TODO
	// struct sg_table * dma_buf_map_attachment(struct dma_buf_attachment * attach, enum dma_data_direction direction)
	// sg_table->dma_address should be the needed bus-address


	mDRMBuffer = (uint8_t*)mmap( 0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset );
	if ( mDRMBuffer == MAP_FAILED ) {
		gError() << "mmap fb failed : " << strerror(errno);
		exit(3);
	}

	mDRMPitch = creq.pitch;
	mDRMWidth = creq.width;
	mDRMHeight = creq.height;
	gDebug() << "DRM buffer : " << mDRMBuffer << "(" << mDRMWidth << "x" << mDRMHeight << ")";
	memset( mDRMBuffer, 0, mDRMPitch * mDRMHeight );
}


DShotDriver::~DShotDriver()
{
}


void DShotDriver::setPinValue( uint32_t pin, uint16_t value, bool telemetry )
{
	value = (value << 1) | telemetry;
	mPinValues[pin] = ( value << 4 ) | ( (value ^ (value >> 4) ^ (value >> 8)) & 0x0F );
	mFlatValues[pin] = false;
}


void DShotDriver::disablePinValue( uint32_t pin )
{
	mFlatValues[pin] = true;
	mPinValues[pin] = 0;
}



void DShotDriver::Update()
{
	uint64_t ticks = Board::GetTicks();

	uint16_t valueFlat[DSHOT_MAX_OUTPUTS] = { 0 };
	uint16_t valueMap[DSHOT_MAX_OUTPUTS] = { 0 };
	uint16_t channelMap[DSHOT_MAX_OUTPUTS] = { 0 };
	uint16_t bitmaskMap[DSHOT_MAX_OUTPUTS] = { 0 };
	uint8_t count = 0;

	for ( auto iter = mPinValues.begin(); iter != mPinValues.end(); ++iter ) {
		valueFlat[count] = mFlatValues[iter->first];
		valueMap[count] = iter->second;
		uint8_t* mapped = sDPIPinMap[sDPIMode][iter->first];
		channelMap[count] = mapped[0];
		bitmaskMap[count] = mapped[1];
		count++;
	}

	constexpr uint32_t bitwidth = 32; // * 6;
	constexpr uint32_t t0h = bitwidth * 1250 / 3333;
	// printf( "t0h : %d\n", t0h);
	uint8_t outbuf[16 * bitwidth * 3];
	memset( outbuf, 0, 16 * bitwidth * 3 );
	// uint8_t* outbuf = mDRMBuffer;
	for ( uint32_t ivalue = 0; ivalue < count; ivalue++ ) {
		// if ( not valueFlat[ivalue] ) {
			uint16_t value = valueMap[ivalue];
			uintptr_t map = channelMap[ivalue];
			uintptr_t bitmask = bitmaskMap[ivalue];
		// printf("[%d] value = %d\n", ivalue, value );
			uint8_t* bPointer = outbuf;
			for ( int8_t i = 15; i >= 0; i-- ) {
				uint8_t repeat = t0h + t0h * ((value >> i) & 1);
				uint8_t rep = repeat;
				while ( rep-- > 0 ) {
					*(bPointer + map) |= ( 1 << bitmask );
					bPointer += 3;
				}
/*
				while ( repeat < bitwidth ) {
					*(bPointer + map) &= ~( 1 << bitmask );
					bPointer += 3;
					repeat++;
				}
*/
				bPointer += 3 * (bitwidth - repeat);
			}
		// }
	}
	// printf( "→ took0 %llu\n", Board::GetTicks() - ticks);

	// uint8_t finalBuf[3072 * 3 * 120];

	for ( uint32_t y = 0; y < mDRMHeight; y++ ) {
		memcpy( mDRMBuffer + y * mDRMPitch, outbuf, 16 * bitwidth * 3 );
	}

	// memcpy( mDRMBuffer, finalBuf, sizeof(finalBuf) );

	// printf( "→ took %llu\n", Board::GetTicks() - ticks);
}

