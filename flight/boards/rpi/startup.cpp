/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <Main.h>

#include <SBUS.h>
#include <BrushlessPWM.h>
#include <PWM.h>
#include <DShot.h>
#include <OneShot42.h>
#include <OneShot125.h>
#include <GPIO.h>
#include <wiringPi.h>
#include <SPI.h>
#include <ICM4xxxx.h>
#include <Gyroscope.h>
void Test( int, char** );


#define _XOPEN_SOURCE 700
#include <fcntl.h> /* open */
#include <stdint.h> /* uint64_t  */
#include <stdio.h> /* printf */
#include <stdlib.h> /* size_t */
#include <unistd.h> /* pread, sysconf */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <xf86drm.h>
#include <xf86drmMode.h>


using namespace std;

std::mutex __global_rpi_drm_mutex;
int __global_rpi_drm_fd = -1;



void fatal(const char *str, int e) {
  fprintf(stderr, "%s: %s\n", str, strerror(e));
  exit(1);
}


typedef struct {
    uint64_t pfn : 55;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
} PagemapEntry;

/* Parse the pagemap entry for the given virtual address.
 *
 * @param[out] entry      the parsed entry
 * @param[in]  pagemap_fd file descriptor to an open /proc/pid/pagemap file
 * @param[in]  vaddr      virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr)
{
    size_t nread;
    ssize_t ret;
    uint64_t data;
    uintptr_t vpn;

    vpn = vaddr / sysconf(_SC_PAGE_SIZE);
    nread = 0;
    while (nread < sizeof(data)) {
        ret = pread(pagemap_fd, ((uint8_t*)&data) + nread, sizeof(data) - nread,
                vpn * sizeof(data) + nread);
        nread += ret;
        if (ret <= 0) {
			printf("err 3\n");
            return 1;
        }
    }
    entry->pfn = data & (((uint64_t)1 << 55) - 1);
    entry->soft_dirty = (data >> 55) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;
    return 0;
}

/* Convert the given virtual address to physical using /proc/PID/pagemap.
 *
 * @param[out] paddr physical address
 * @param[in]  pid   process to convert for
 * @param[in] vaddr virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int virt_to_phys_user(uintptr_t *paddr, uintptr_t vaddr)
{
	errno = 0;
    char pagemap_file[BUFSIZ];
    int pagemap_fd;

    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%u/pagemap", getpid());
	printf("pagemap_file: %s\n", pagemap_file);
    pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
		printf("err 1\n");
        return 1;
    }
    PagemapEntry entry;
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
		printf("err 2\n");
        return 1;
    }
    close(pagemap_fd);
    *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
	printf("end\n");
    return 0;
}




void *setupFrameBuffer(int fd, int crtc_id, uint32_t conn_id, uint32_t *width, uint32_t *heigth, uint32_t *pitch) {
  struct drm_mode_create_dumb creq;
  struct drm_mode_map_dumb mreq;
  uint32_t fb_id;
  void *buf;
  drmModeCrtcPtr crtc_info = drmModeGetCrtc(fd, crtc_id);

  memset(&creq, 0, sizeof(struct drm_mode_create_dumb));
  creq.width = crtc_info->mode.hdisplay;
  creq.height = crtc_info->mode.vdisplay;
  creq.bpp = 24;
  printf("%d x %d\n", creq.width, creq.height);

  if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
    fatal("drmIoctl DRM_IOCTL_MODE_CREATE_DUMB failed", errno);
  }

  printf("drmModeAddFB(%d, %d, %d, 24, 24, %d, %d, fb_id)\n", fd, creq.width, creq.height, creq.pitch, creq.handle);
  if (drmModeAddFB(fd, creq.width, creq.height, 24, 24, creq.pitch, creq.handle, &fb_id)) {
    fatal("drmModeAddFB failed", errno);
  }
  printf("fb_id: %d\n", fb_id);
  printf("pitch: %d\n", creq.pitch);

  memset(&mreq, 0, sizeof(struct drm_mode_map_dumb));
  mreq.handle = creq.handle;

  if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq)) {
    fatal("drmIoctl DRM_IOCTL_MODE_MAP_DUMB failed", errno);
  }

  buf = (void *) mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);
  if (buf == MAP_FAILED) {
    perror("cant mmap fb");
    exit(2);
  }

  if (drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0, &conn_id, 1, &crtc_info->mode)) {
    fatal("drmModeSetCrtc() failed", errno);
  }
  drmModeFreeCrtc(crtc_info);
  *width = creq.width;
  *heigth = creq.height;
  *pitch = creq.pitch;
  return buf;
}


string connectorTypeToStr(uint32_t type) {
  switch (type) {
  case DRM_MODE_CONNECTOR_HDMIA: // 11
    return "HDMIA";
  case DRM_MODE_CONNECTOR_DSI: // 16
    return "DSI";
  }
  return "unknown";
}

void printDrmModes(int fd) {
  drmVersionPtr version = drmGetVersion(fd);
  printf("version %d.%d.%d\nname: %s\ndate: %s\ndescription: %s\n", version->version_major, version->version_minor, version->version_patchlevel, version->name, version->date, version->desc);
  drmFreeVersion(version);
  drmModeRes * modes = drmModeGetResources(fd);
  printf("count_fbs: %d\n", modes->count_fbs);
  for (int i=0; i < modes->count_fbs; i++) {
    printf("FB#%d: %x\n", i, modes->fbs[i]);
  }
  for (int i=0; i < modes->count_crtcs; i++) {
    printf("CRTC#%d: %d\n", i, modes->crtcs[i]);
    drmModeCrtcPtr crtc = drmModeGetCrtc(fd, modes->crtcs[i]);
    printf("  buffer_id: %d\n", crtc->buffer_id);
    printf("  position: %dx%d\n", crtc->x, crtc->y);
    printf("  size: %dx%d\n", crtc->width, crtc->height);
    printf("  mode_valid: %d\n", crtc->mode_valid);
    printf("  gamma_size: %d\n", crtc->gamma_size);
    printf("  Mode\n    clock: %d\n", crtc->mode.clock);
    drmModeModeInfo &mode = crtc->mode;
    printf("    h timings: %d %d %d %d %d\n", mode.hdisplay, mode.hsync_start, mode.hsync_end, mode.htotal, mode.hskew);
    printf("    v timings: %d %d %d %d %d\n", mode.vdisplay, mode.vsync_start, mode.vsync_end, mode.vtotal, mode.vscan);
    printf("    vrefresh: %d\n", mode.vrefresh);
    printf("    flags: 0x%x\n", mode.flags);
    printf("    type: %d\n", mode.type);
    printf("    name: %s\n", mode.name);
    drmModeFreeCrtc(crtc);
  }
  for (int i=0; i < modes->count_connectors; i++) {
    printf("Connector#%d: %d\n", i, modes->connectors[i]);
    drmModeConnectorPtr connector = drmModeGetConnector(fd, modes->connectors[i]);
    if (connector->connection == DRM_MODE_CONNECTED) puts("  connected!");
    string typeStr = connectorTypeToStr(connector->connector_type);
    printf("  ID: %d\n  Encoder: %d\n  Type: %d %s\n  type_id: %d\n  physical size: %dx%d\n", connector->connector_id, connector->encoder_id, connector->connector_type, typeStr.c_str(), connector->connector_type_id, connector->mmWidth, connector->mmHeight);
    for (int j=0; j < connector->count_encoders; j++) {
      printf("  Encoder#%d:\n", j);
      drmModeEncoderPtr enc = drmModeGetEncoder(fd, connector->encoders[j]);
      printf("    ID: %d\n    Type: %d\n    CRTCs: 0x%x\n    Clones: 0x%x\n", enc->encoder_id, enc->encoder_type, enc->possible_crtcs, enc->possible_clones);
      drmModeFreeEncoder(enc);
    }
    printf("  Modes: %d\n", connector->count_modes);
    for (int j=0; j < connector->count_modes; j++) {
      printf("  Mode#%d:\n", j);
      if (j > 1) break;
      drmModeModeInfo &mode = connector->modes[j];
      printf("    clock: %d\n", mode.clock);
      printf("    h timings: %d %d %d %d %d\n", mode.hdisplay, mode.hsync_start, mode.hsync_end, mode.htotal, mode.hskew);
      printf("    v timings: %d %d %d %d %d\n", mode.vdisplay, mode.vsync_start, mode.vsync_end, mode.vtotal, mode.vscan);
      printf("    vrefresh: %d\n", mode.vrefresh);
      printf("    flags: 0x%x\n", mode.flags);
      printf("    type: %d\n", mode.type);
      printf("    name: %s\n", mode.name);
    }
    drmModeFreeConnector(connector);
  }
  for (int i=0; i < modes->count_encoders; i++) {
    printf("Encoder#%d: %d\n", i, modes->encoders[i]);
  }
  printf("min size: %dx%d\n", modes->min_width, modes->min_height);
  printf("max size: %dx%d\n", modes->max_width, modes->max_height);
  drmModeFreeResources(modes);
}


/* emits uart data over DPI
 * config.txt contents:
dpi_output_format=0x17
dpi_group=2
dpi_mode=87
dpi_timings=50 0 0 1 0 2000 0 0 1 0 0 0 0 30 0 5000000 6
dtoverlay=dpi24
enable_dpi_lcd=1
 */


uint16_t dshot_conv( uint16_t v, bool telemetry = 0 ) {
	v = (v << 1) | telemetry;
	return ( v << 4 ) | ( (v ^ (v >> 4) ^ (v >> 8)) & 0x0F );
}


void emitSineData(void *buf, uint32_t width, uint32_t height, uint32_t pitch, uint16_t value) {

	uint64_t ticks = Board::GetTicks();
	uint8_t test[480*3];

	uint16_t values[4] = {
		dshot_conv(value),
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111
	};
	uint8_t channel_map[4] = {
		1,
		0,
		0,
		0
	};
	uint8_t bitmask_map[4] = {
		0,
		2,
		0,
		0
	};
	struct {
		uint8_t r[16 * 32];
		uint8_t g[16 * 32];
		uint8_t b[16 * 32];
	} valuebuf;
	int n = 0;
	memset(valuebuf.r, 0, 3 * 16 * 32);
	for ( uint32_t ivalue = 0; ivalue < 4; ivalue++ ) {
		uint16_t value = values[ivalue];
		uintptr_t map = channel_map[ivalue];
		uintptr_t bitmask = bitmask_map[ivalue];
		uint8_t* bPointer = valuebuf.r;
		for ( int8_t i = 15; i >= 0; i-- ) {
			uint8_t repeat = 12 + 12 * ((value >> i) & 1);
			uint8_t rep = repeat;
			while ( rep-- > 0 ) {
				*(bPointer + map) |= ( 1 << bitmask );
				bPointer += 3;
			}
			bPointer += 3 * (32 - repeat);
		}
	}

	for (uint32_t y=0; y < height; y++) {
		memcpy((char*)buf + y*pitch, valuebuf.r, 16 * 32 * 3);
	}
	printf( "→ took %llu\n", Board::GetTicks() - ticks);

	return;
}


uint32_t width,height,pitch;
void *buf;

int test() {
	int fd = -1;
	if (__global_rpi_drm_fd < 0) {
		fd = open("/dev/dri/card1", O_RDWR);
		__global_rpi_drm_fd = fd;
	} else {
		fd = __global_rpi_drm_fd;
	}
	uint32_t crtc_id, connector_id;
	crtc_id = 87;
	connector_id = 89;
	if (fd < 0) {
		perror("unable to open /dev/dri/card1");
		exit(3);
	}
	uint64_t has_dumb;
	if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || has_dumb == 0) {
		fatal("drmGetCap DRM_CAP_DUMB_BUFFER failed or doesn't have dumb buffer", -1);
	}
	drmSetMaster(fd);
	printDrmModes(fd);
	buf = setupFrameBuffer(fd, crtc_id, connector_id, &width, &height, &pitch);
	printf( "width, height, pitch : %d, %d, %d\n", width, height, pitch );
	//emitUartData(buf,"Uart test data", width, height, pitch);

	uintptr_t pbuf = 0;
	int r = virt_to_phys_user(&pbuf, (uintptr_t)buf);
	if ( r < 0 ) {
		perror("virt_to_phys_user\n");
	}
	printf( "pbuf : %p, %p (%s)\n", buf, pbuf, strerror(errno) );

	memset(buf, 0, pitch*height);
	return 0;
}

class TestThread : public Thread {
public:
	TestThread() : Thread() {}
	virtual ~TestThread() {}
protected:
	bool run() {
		/*
		usleep(10 * 1000 * 1000);
		test();
		emitSineData(buf, width, height, pitch, 0);
		sleep(5);
		int val = 48;
		int way = 1;
		while ( true ) {
			val += way;
			if ( val < 48 ) {
				val = 48;
				way = 1;
				sleep(1);
			}
			if ( val > 1000 ) {
				val = 1000;
				way = -1;
				sleep(1);
			}
			printf("%d\n", val);
			emitSineData(buf, width, height, pitch, val);
		}
		*/
		DShot* m = new DShot(12);
		m->Disarm();
		sleep(5);
		m->setSpeed(0.25, true);

		while ( true ) {
			usleep( 1000 );
		}
		return false;
	}
};



int main( int ac, char** av )
{
	Debug::setDebugLevel( Debug::Verbose );
	wiringPiSetup();
	wiringPiSetupGpio();
/*
		DShot* m = new DShot(5);
		m->Disarm();
		sleep(5);
		m->setSpeed(0.15, true);
		sleep(2);
		return 0;
*/
	// test();
	// emitSineData(buf, width, height, pitch, 0);
	// while ( true ) {
	// 	sleep(1);
	// }
	// TestThread t;
	// t.Start();


	if ( 0 ) {
		SPI* spi = new SPI( "/dev/spidev3.0", 16000000 );

		uint8_t dummy[10] = { 0 };
		uint8_t tx[9] = { 0x42, 1, 2, 3, 4, 5, 6, 7, 8 };
		uint8_t r_tx[9] = { 0b10000000 | 0x42, 0, 0, 0, 0, 0, 0, 0, 0 };
		uint8_t _r_rx[9] = { 0 };
		uint8_t* r_rx = _r_rx + 1;
	// 	uint8_t r_rx[9] = { 0 };
		uint64_t t = 0;

		uint64_t fpsCounter = Board::GetTicks();
		uint32_t fpsI = 0;
		uint32_t fps = 0;

		printf( "Send reset\n" );
		spi->Write( { 0x01, 1, 1, 1, 1, 1, 1, 1, 1 } );
		usleep( 1000 * 1000 );

		printf( "Send resetaaa\n" );
		spi->Write( { 0x01, 1, 1, 1, 1, 1, 1, 1, 1 } );
		usleep( 1000 * 1000 );

		printf( "Send REG_PWM_CONFIG\n" );
		spi->Write( { 0x28, 0b00000000, 0, 0, 0, 0, 0, 0, 0 } );
		usleep( 1000 * 1000 );

/*
	#define MIN 1000
	#define LEN 1000
	#define ARM 1060
	#define MAX 1860
*/
/*
	#define MIN 40
	#define LEN 44
	#define ARM 42
	#define MAX 83
*/
	#define MIN 42
	#define LEN 42
	#define ARM 44
	#define MAX 83

		printf( "Send minval and maxval\n" );
		spi->Write<uint32_t>( 0x30, { MIN, LEN } );
		usleep( 1000 * 1000 * 2 );

		printf( "Send REG_DEVICE_CONFIG\n" );
		spi->Write( { 0x20, 0b00000001, 0, 0, 0, 0, 0, 0, 0 } );
		usleep( 1000 * 1000 );

		uint16_t n = ( MAX - MIN ) * 65535 / LEN;

		spi->Write( { 0x70, 0, 0, 0, 0, 0, 0, 0, 0 } );
		usleep( 1000 * 100 );

		if ( 0 ) {

			printf( "Set max (%d)\n", n );
			memcpy( &tx[1], &n, 2 );
			memcpy( &tx[3], &n, 2 );
			memcpy( &tx[5], &n, 2 );
			memcpy( &tx[7], &n, 2 );
			spi->Transfer( tx, dummy, 9 );

// 			usleep( 1000 * 1000 * 8 );
			printf( "Press any key to continue\n" );
			{ char c; read( 0, &c, 1 ); }

			n = ( ARM - MIN ) * 65535 / LEN;
			printf( "Set min (%d)\n", n );
			memcpy( &tx[1], &n, 2 );
			memcpy( &tx[3], &n, 2 );
			memcpy( &tx[5], &n, 2 );
			memcpy( &tx[7], &n, 2 );
			spi->Transfer( tx, dummy, 9 );
// 			usleep( 1000 * 1000 * 6 );
			printf( "Press any key to continue\n" );
			{ char c; read( 0, &c, 1 ); }
		}

		printf( "Send REG_DEVICE_CONFIG\n" );
		spi->Write( { 0x20, 0b00000101, 0, 0, 0, 0, 0, 0, 0 } );


		usleep( 1000 * 1000 * 2 );
		uint16_t v = ( ARM - MIN ) * 65535 / LEN;
		printf( "Initial value : %u\n", v );
		memcpy( &tx[1], &v, 2 );
		memcpy( &tx[3], &v, 2 );
		memcpy( &tx[5], &v, 2 );
		memcpy( &tx[7], &v, 2 );
		spi->Transfer( tx, dummy + 1, 9 );
		usleep( 1000 * 1000 );
		while ( 1 ) {
			t = Board::GetTicks();
			memcpy( &tx[1], &v, 2 );
			memcpy( &tx[3], &v, 2 );
			memcpy( &tx[5], &v, 2 );
			memcpy( &tx[7], &v, 2 );
// 			if ( v < 8192 ) {
				v += 20;
// 			}
			spi->Transfer( tx, dummy + 1, 9 );
			t = Board::GetTicks() - t;

			printf( "[%lluµs tx][%d FPS] %u (→ %04X %04X %04X %04X)\n", t, fps, v, ((uint16_t*)dummy)[1], ((uint16_t*)dummy)[2], ((uint16_t*)dummy)[3], ((uint16_t*)dummy)[4] );

			fpsI++;
			if ( Board::GetTicks() - fpsCounter >= 1000000 ) {
				fpsCounter = Board::GetTicks();
				fps = fpsI;
				fpsI = 0;
	// 			printf( "%d\n", fps );
			}

			usleep( 1000 * 12 );
		}
	}
/*
	PWM::EnableTruePWM();

	GPIO::setMode(6, GPIO::Output);
	GPIO::setMode(12, GPIO::Output);
	GPIO::Write(6, 1);
	PWM* pwm = new PWM( 12, 10000000, 2000, PWM::MODE_PWM );
	// PWM* pwm = new PWM( 12, 1000000, 2000, 2, PWM::MODE_PWM );
	pwm->SetPWMus( 500 );
	pwm->Update();
	while ( true ) {
		// GPIO::Write(12, 1);
		usleep( 5 * 1000 );
		// GPIO::Write(12, 0);
		usleep( 5 * 1000 );
	}
*/
/*
	Motor* test1 = new OneShot125( 12, 125, 250-8, pwm );
	// Motor* test2 = new BrushlessPWM( 13 );
	// Motor* test3 = new BrushlessPWM( 0 );
	// Motor* test4 = new BrushlessPWM( 1 );
// 	test1->setSpeed( 1.0f );
// 	test2->setSpeed( 1.0f );
// 	test3->setSpeed( 1.0f );
// 	test4->setSpeed( 1.0f, true );
// 	usleep( 5 * 1000 * 1000 );
	test1->setSpeed( 0.0f, true );
	// test2->setSpeed( 0.0f );
	// test3->setSpeed( 0.0f );
	// test4->setSpeed( 0.0f, true );
// 	usleep( 8 * 1000 * 1000 );
	usleep( 1000 * 1000 );
	float sp = 0.0f;
	float d = 0.01f;
	while ( 1 ) {
		sp += d;
		if ( sp < 0.0f or sp > 1.0f ) {
			d = -d;
			sp += d;
		}
		printf( "sp : %.2f\n", sp );
		test1->setSpeed( sp, true );
		// test2->setSpeed( sp );
		// test3->setSpeed( sp );
		// test4->setSpeed( sp, true );
		usleep( 500 * 1000 );
	}
*/
/*
	Bus* bus = new SPI( "/dev/spidev0.0" );
	Gyroscope* gyro = dynamic_cast<Gyroscope*>( ICM4xxxx::Instanciate( bus, nullptr, "" ) );
	while ( 1 ) {
		Vector3f v;
		gyro->Read( &v, true );
		printf( "%08.2f %08.2f %08.2f\n", v.x, v.y, v.z );
		usleep( 1000 * 10 );
	}
*/
	int ret = Main::flight_entry( ac, av );

	if ( ret == 0 ) {
		while ( 1 ) {
			usleep( 1000 * 1000 * 100 );
		}
	}

	return 0;
}
