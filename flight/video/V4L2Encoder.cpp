#include <sys/fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include <map>

#include "V4L2Encoder.h"
#include "Debug.h"


static constexpr int NUM_OUTPUT_BUFFERS = 6;
static constexpr int NUM_CAPTURE_BUFFERS = 12;

static int xioctl( int fd, unsigned long ctl, void* arg )
{
	int ret, num_tries = 10;

	do {
		ret = ioctl( fd, ctl, arg );
	} while (ret == -1 && errno == EINTR && num_tries-- > 0);

	return ret;
}


V4L2Encoder::V4L2Encoder()
	: VideoEncoder()
	, mVideoDevice( "/dev/video11" )
	, mBitrate( 4 * 1024 )
	, mWidth( 1280 )
	, mHeight( 720 )
	, mFramerate( 30 )
	, mFD( -1 )
{
}


V4L2Encoder::~V4L2Encoder()
{
}


void V4L2Encoder::Setup()
{
	mFD = open( mVideoDevice.c_str(), O_RDWR, 0 );
	if (mFD < 0) {
		throw std::runtime_error("failed to open V4L2 H264 encoder");
	}
	gDebug() << "Opened H264Encoder on " << mVideoDevice << " as fd " << mFD;

	v4l2_control ctrl = {};
	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
		ctrl.value = mBitrate;
		if (xioctl(mFD, VIDIOC_S_CTRL, &ctrl) < 0) {
			throw std::runtime_error("failed to set bitrate");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
		ctrl.value = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH;
		if (xioctl(mFD, VIDIOC_S_CTRL, &ctrl) < 0) {
			throw std::runtime_error("failed to set profile");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
		ctrl.value = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
		if (xioctl(mFD, VIDIOC_S_CTRL, &ctrl) < 0) {
			throw std::runtime_error("failed to set level");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
		ctrl.value = 10;
		if (xioctl(mFD, VIDIOC_S_CTRL, &ctrl) < 0) {
			throw std::runtime_error("failed to set intra period");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
		ctrl.value = 1; // Inline headers
		if (xioctl(mFD, VIDIOC_S_CTRL, &ctrl) < 0) {
			throw std::runtime_error("failed to set inline headers");
		}
	}

	// Set the output and capture formats. We know exactly what they will be.

	v4l2_format fmt = {};
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.width = mWidth;
	fmt.fmt.pix_mp.height = mHeight;
	// We assume YUV420 here, but it would be nice if we could do something
	// like info.pixel_format.toV4L2Fourcc();
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix_mp.plane_fmt[0].bytesperline = mWidth; // On YUV420, stride == width
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
// 	fmt.fmt.pix_mp.colorspace = get_v4l2_colorspace(info.colour_space); // TOD
	fmt.fmt.pix_mp.num_planes = 1;
	if (xioctl(mFD, VIDIOC_S_FMT, &fmt) < 0) {
		throw std::runtime_error("failed to set output format");
	}

	fmt = {};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = mWidth;
	fmt.fmt.pix_mp.height = mHeight;
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
	fmt.fmt.pix_mp.num_planes = 1;
	fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
	fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 512 << 10;
	if (xioctl(mFD, VIDIOC_S_FMT, &fmt) < 0) {
		throw std::runtime_error("failed to set capture format");
	}

	struct v4l2_streamparm parm = {};
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	parm.parm.output.timeperframe.numerator = 1000 / mFramerate;
	parm.parm.output.timeperframe.denominator = 1000;
	if (xioctl(mFD, VIDIOC_S_PARM, &parm) < 0) {
		throw std::runtime_error("failed to set streamparm");
	}

	// Request that the necessary buffers are allocated. The output queue
	// (input to the encoder) shares buffers from our caller, these must be
	// DMABUFs. Buffers for the encoded bitstream must be allocated and
	// m-mapped.

	v4l2_requestbuffers reqbufs = {};
	reqbufs.count = NUM_OUTPUT_BUFFERS;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	reqbufs.memory = V4L2_MEMORY_DMABUF;
	if (xioctl(mFD, VIDIOC_REQBUFS, &reqbufs) < 0) {
		throw std::runtime_error("request for output buffers failed");
	}
	gDebug() << "Got " << reqbufs.count << " output buffers";

	// We have to maintain a list of the buffers we can use when our caller gives
	// us another frame to encode.
	for (unsigned int i = 0; i < reqbufs.count; i++) {
		input_buffers_available_.push(i);
	}

	reqbufs = {};
	reqbufs.count = NUM_CAPTURE_BUFFERS;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	if (xioctl(mFD, VIDIOC_REQBUFS, &reqbufs) < 0) {
		throw std::runtime_error("request for capture buffers failed");
	}
	gDebug() << "Got " << reqbufs.count << " capture buffers";
	num_capture_buffers_ = reqbufs.count;

	for ( uint32_t i = 0; i < reqbufs.count; i++ ) {
		v4l2_plane planes[VIDEO_MAX_PLANES];
		v4l2_buffer buffer = {};
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		buffer.length = 1;
		buffer.m.planes = planes;
		if (xioctl(mFD, VIDIOC_QUERYBUF, &buffer) < 0) {
			throw std::runtime_error("failed to capture query buffer " + std::to_string(i));
		}
		buffers_[i].mem = mmap(0, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, mFD, buffer.m.planes[0].m.mem_offset);
		if (buffers_[i].mem == MAP_FAILED) {
			throw std::runtime_error("failed to mmap capture buffer " + std::to_string(i));
		}
		buffers_[i].size = buffer.m.planes[0].length;
		// Whilst we're going through all the capture buffers, we may as well queue
		// them ready for the encoder to write into.
		if (xioctl(mFD, VIDIOC_QBUF, &buffer) < 0) {
			throw std::runtime_error("failed to queue capture buffer " + std::to_string(i));
		}
	}

	// Enable streaming and we're done.

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if ( xioctl( mFD, VIDIOC_STREAMON, &type ) < 0 ) {
		throw std::runtime_error("failed to start output streaming");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if (xioctl(mFD, VIDIOC_STREAMON, &type) < 0) {
		throw std::runtime_error("failed to start capture streaming");
	}
	gDebug() << "Codec streaming started";

	output_thread_ = std::thread(&H264Encoder::outputThread, this);
	poll_thread_ = std::thread(&H264Encoder::pollThread, this);
}

