#include <sys/fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include <map>
#include <chrono>

#include "V4L2Encoder.h"
#include "Debug.h"


static constexpr int NUM_INPUT_BUFFERS = 6;
static constexpr int NUM_OUTPUT_BUFFERS = 12;

static int xioctl( int fd, unsigned long ctl, void* arg )
{
	int ret, num_tries = 10;

	do {
		ret = ioctl( fd, ctl, arg );
	} while (ret == -1 && errno == EINTR && num_tries-- > 0 );

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
	if ( mFD < 0 ) {
		throw std::runtime_error("failed to open V4L2 H264 encoder");
	}
	gDebug() << "Opened H264Encoder on " << mVideoDevice << " as fd " << mFD;

	v4l2_control ctrl = {};
	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
		ctrl.value = mBitrate;
		if ( xioctl( mFD, VIDIOC_S_CTRL, &ctrl ) < 0 ) {
			throw std::runtime_error("failed to set bitrate");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
		ctrl.value = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH;
		if ( xioctl( mFD, VIDIOC_S_CTRL, &ctrl ) < 0 ) {
			throw std::runtime_error("failed to set profile");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
		ctrl.value = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
		if ( xioctl( mFD, VIDIOC_S_CTRL, &ctrl ) < 0 ) {
			throw std::runtime_error("failed to set level");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
		ctrl.value = 10;
		if ( xioctl( mFD, VIDIOC_S_CTRL, &ctrl ) < 0 ) {
			throw std::runtime_error("failed to set intra period");
		}
	}

	{
		ctrl.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
		ctrl.value = 1; // Inline headers
		if ( xioctl( mFD, VIDIOC_S_CTRL, &ctrl ) < 0 ) {
			throw std::runtime_error("failed to set inline headers");
		}
	}

	// Set the input and output formats. We know exactly what they will be.

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
	if ( xioctl( mFD, VIDIOC_S_FMT, &fmt ) < 0 ) {
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
	if ( xioctl( mFD, VIDIOC_S_FMT, &fmt ) < 0 ) {
		throw std::runtime_error("failed to set output format");
	}

	struct v4l2_streamparm parm = {};
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	parm.parm.output.timeperframe.numerator = 1000 / mFramerate;
	parm.parm.output.timeperframe.denominator = 1000;
	if ( xioctl( mFD, VIDIOC_S_PARM, &parm ) < 0 ) {
		throw std::runtime_error("failed to set streamparm");
	}

	// Request that the necessary buffers are allocated. The output queue
	// (input to the encoder) shares buffers from our caller, these must be
	// DMABUFs. Buffers for the encoded bitstream must be allocated and
	// m-mapped.

	v4l2_requestbuffers reqbufs = {};
	reqbufs.count = NUM_INPUT_BUFFERS;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	reqbufs.memory = V4L2_MEMORY_DMABUF;
	if ( xioctl( mFD, VIDIOC_REQBUFS, &reqbufs ) < 0 ) {
		throw std::runtime_error("request for input buffers failed");
	}
	gDebug() << "Got " << reqbufs.count << " input buffers";

	// We have to maintain a list of the buffers we can use when our caller gives
	// us another frame to encode.
	for (unsigned int i = 0; i < reqbufs.count; i++) {
		mInputBuffersAvailable.push(i);
	}

	reqbufs = {};
	reqbufs.count = NUM_OUTPUT_BUFFERS;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	if ( xioctl( mFD, VIDIOC_REQBUFS, &reqbufs ) < 0 ) {
		throw std::runtime_error("request for output buffers failed");
	}
	gDebug() << "Got " << reqbufs.count << " output buffers";
	mOutputBuffersCount = reqbufs.count;

	for ( uint32_t i = 0; i < reqbufs.count; i++ ) {
		v4l2_plane planes[VIDEO_MAX_PLANES];
		v4l2_buffer buffer = {};
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		buffer.length = 1;
		buffer.m.planes = planes;
		if ( xioctl( mFD, VIDIOC_QUERYBUF, &buffer ) < 0 ) {
			throw std::runtime_error("failed to output query buffer " + std::to_string(i));
		}
		BufferDescription outputBuf = {
			.mem = mmap( 0, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, mFD, buffer.m.planes[0].m.mem_offset ),
			.size = buffer.m.planes[0].length
		};
		if ( outputBuf.mem == MAP_FAILED ) {
			throw std::runtime_error("failed to mmap output buffer " + std::to_string(i));
		}
		mOutputBuffers.push_back( outputBuf );
		// Whilst we're going through all the output buffers, we may as well queue
		// them ready for the encoder to write into.
		if ( xioctl( mFD, VIDIOC_QBUF, &buffer ) < 0 ) {
			throw std::runtime_error("failed to queue output buffer " + std::to_string(i));
		}
	}

	// Enable streaming and we're done.

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if ( xioctl( mFD, VIDIOC_STREAMON, &type ) < 0 ) {
		throw std::runtime_error("failed to start output streaming");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if ( xioctl( mFD, VIDIOC_STREAMON, &type ) < 0 ) {
		throw std::runtime_error("failed to start output streaming");
	}
	gDebug() << "Codec streaming started";

	if ( mRecorder ) {
		mRecorderTrackId = mRecorder->AddVideoTrack( mWidth, mHeight, mFramerate, "H264" );
	}

// 	mOutputThread = std::thread( &V4L2Encoder::outputThread, this );
	mPollThread = std::thread( &V4L2Encoder::pollThread, this );
}


void V4L2Encoder::EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd )
{
	int index;

	{
		// We need to find an available output buffer (input to the codec) to
		// "wrap" the DMABUF.
		std::lock_guard<std::mutex> lock(mInputBuffersAvailableMutex);
		if ( mInputBuffersAvailable.empty() ) {
			throw std::runtime_error("no buffers available to queue codec input");
		}
		index = mInputBuffersAvailable.front();
		mInputBuffersAvailable.pop();
	}

	v4l2_buffer buf = {};
	v4l2_plane planes[VIDEO_MAX_PLANES] = {};
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.index = index;
	buf.field = V4L2_FIELD_NONE;
	buf.memory = V4L2_MEMORY_DMABUF;
	buf.length = 1;
	buf.timestamp.tv_sec = timestamp_us / 1000000;
	buf.timestamp.tv_usec = timestamp_us % 1000000;
	buf.m.planes = planes;
	buf.m.planes[0].m.fd = fd;
	buf.m.planes[0].bytesused = size;
	buf.m.planes[0].length = size;

	if ( xioctl( mFD, VIDIOC_QBUF, &buf ) < 0 ) {
		throw std::runtime_error("failed to queue input to codec");
	}
}


void V4L2Encoder::pollThread()
{
	while ( true ) {
		pollfd p = { mFD, POLLIN, 0 };
		int ret = poll(&p, 1, 200);
		{
			std::lock_guard<std::mutex> lock(mInputBuffersAvailableMutex);
			if ( mInputBuffersAvailable.size() == NUM_OUTPUT_BUFFERS ) {
				break;
			}
		}
		if ( ret < 0 ) {
			if ( errno == EINTR ) {
				continue;
			}
			throw std::runtime_error("unexpected errno " + std::to_string(errno) + " from poll");
		}
		if ( p.revents & POLLIN ) {
			v4l2_plane planes[VIDEO_MAX_PLANES] = {};
			memset(planes, 0, sizeof(planes));
			v4l2_buffer buf = {};
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			buf.memory = V4L2_MEMORY_DMABUF;
			buf.length = 1;
			buf.m.planes = planes;
			int ret = xioctl( mFD, VIDIOC_DQBUF, &buf );
			if ( ret == 0 ) {
				// Return this to the caller, first noting that this buffer, identified
				// by its index, is available for queueing up another frame.
				{
					std::lock_guard<std::mutex> lock(mInputBuffersAvailableMutex);
					mInputBuffersAvailable.push(buf.index);
				}
			}

			buf = {};
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.length = 1;
			buf.m.planes = planes;
			ret = xioctl( mFD, VIDIOC_DQBUF, &buf );
			if ( ret == 0 ) {
				// We push this encoded buffer to another thread so that our
				// application can take its time with the data without blocking the
				// encode process.s
				int64_t timestamp_us = (buf.timestamp.tv_sec * (int64_t)1000000) + buf.timestamp.tv_usec;
				if ( mRecorder and mRecorderTrackId != -1 ) {
					mRecorder->WriteSample( mRecorderTrackId, timestamp_us, mOutputBuffers[buf.index].mem, buf.m.planes[0].bytesused );
				}
				/*
				OutputItem item = {
					mOutputBuffers[buf.index].mem,
					buf.m.planes[0].bytesused,
					buf.m.planes[0].length,
					buf.index,
					!!(buf.flags & V4L2_BUF_FLAG_KEYFRAME),
					timestamp_us
				};
				std::lock_guard<std::mutex> lock(mOutputMutex);
				mOutputQueue.push(item);
				mOutputCondVar.notify_one();
				*/
			}
		}
	}
}


void V4L2Encoder::outputThread()
{
/*
	OutputItem item;
	while ( true ) {
		{
			std::unique_lock<std::mutex> lock(mOutputMutex);
			while ( true ) {
// 				if (abortOutput_ && mOutputQueue.empty())
// 					return;

				if ( !mOutputQueue.empty() ) {
					item = mOutputQueue.front();
					mOutputQueue.pop();
					break;
				} else {
					mOutputCondVar.wait_for( lock, std::chrono::milliseconds(200) );
				}
			}
		}

// 		output_ready_callback_(item.mem, item.bytes_used, item.timestamp_us, item.keyframe);

		v4l2_buffer buf = {};
		v4l2_plane planes[VIDEO_MAX_PLANES] = {};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = item.index;
		buf.length = 1;
		buf.m.planes = planes;
		buf.m.planes[0].bytesused = 0;
		buf.m.planes[0].length = item.length;
		if ( xioctl( mFD, VIDIOC_QBUF, &buf) < 0 ) {
			throw std::runtime_error("failed to re-queue encoded buffer");
		}
	}
*/
}
