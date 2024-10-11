extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/frame.h>
	#include <libavutil/hwcontext_drm.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/opt.h>
}
#include "AvcodecEncoder.h"
#include "Debug.h"
#include <libdrm/drm_fourcc.h>

static const std::map< AvcodecEncoder::Format, std::string > formatToString = {
	{ AvcodecEncoder::FORMAT_H264, "h264" },
	{ AvcodecEncoder::FORMAT_MJPEG, "mjpeg" }
};


AvcodecEncoder::AvcodecEncoder()
	: VideoEncoder()
{
}


AvcodecEncoder::~AvcodecEncoder()
{
}


void AvcodecEncoder::Setup()
{
	fDebug();
	mReady = true;
	avformat_network_init();

	// Find the H.264 encoder
	mCodec = avcodec_find_encoder( AV_CODEC_ID_H264 );
	if ( !mCodec ) {
		gError() << "Codec not found";
		return;
	}

	// Allocate codec context
	mCodecContext = avcodec_alloc_context3( mCodec );
	if ( !mCodecContext ) {
		gError() << "Could not allocate video codec context";
		return;
	}

	// Set codec parameters
	mCodecContext->bit_rate = mBitrate;
	mCodecContext->width = mWidth;
	mCodecContext->height = mHeight;
	mCodecContext->time_base = (AVRational){ 1, 1000 * 1000 };
	mCodecContext->framerate = (AVRational){ mFramerate * 1000, 1000 };
	mCodecContext->gop_size = 10;
	mCodecContext->max_b_frames = 0;
	mCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	mCodecContext->sw_pix_fmt = AV_PIX_FMT_YUV420P;
	mCodecContext->me_range = 16;
	mCodecContext->me_cmp = 1; // No chroma ME
	mCodecContext->me_subpel_quality = 0;
	mCodecContext->thread_count = 0;
	mCodecContext->thread_type = FF_THREAD_FRAME;
	mCodecContext->slices = 1;
	av_opt_set( mCodecContext->priv_data, "preset", "superfast", 0 );
	av_opt_set( mCodecContext->priv_data, "partitions", "i8x8,i4x4", 0 );
	av_opt_set( mCodecContext->priv_data, "weightp", "none", 0 );
	av_opt_set( mCodecContext->priv_data, "weightb", "0", 0 );
	av_opt_set( mCodecContext->priv_data, "motion-est", "dia", 0 );
	av_opt_set( mCodecContext->priv_data, "sc_threshold", "0", 0 );
	av_opt_set( mCodecContext->priv_data, "rc-lookahead", "0", 0 );
	av_opt_set( mCodecContext->priv_data, "mixed_ref", "0", 0 );

	if ( avcodec_open2( mCodecContext, mCodec, nullptr ) < 0 ) {
		gError() << "Could not open codec";
		return;
	}

	if ( mRecorder ) {
		mRecorderTrackId = mRecorder->AddVideoTrack( formatToString.at(mFormat), mWidth, mHeight, mFramerate, formatToString.at(mFormat) );
	}
}


void AvcodecEncoder::EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd )
{
	if ( not mReady ) {
		Setup();
		usleep( 10 * 1000 );
	}

	AVFrame *frame = av_frame_alloc();
	frame->format = AV_PIX_FMT_YUV420P;
	frame->width = mWidth;
	frame->height = mHeight;
	frame->linesize[0] = mWidth;
	frame->linesize[1] = frame->linesize[2] = mWidth >> 1;
	frame->pts = timestamp_us;
	frame->buf[0] = av_buffer_create( (uint8_t *)mem, size, nullptr, nullptr, 0 );
	av_image_fill_pointers( frame->data, AV_PIX_FMT_YUV420P, frame->height, frame->buf[0]->data, frame->linesize );
	av_frame_make_writable( frame );


	AVPacket packet;
	av_init_packet( &packet );
	packet.data = NULL;
	packet.size = 0;

	int ret = avcodec_send_frame( mCodecContext, frame );
	if ( ret < 0 ) {
		gError() << "Error sending frame to codec context (" << ret << ")";
		return;
	}

	ret = avcodec_receive_packet( mCodecContext, &packet );
	if (ret < 0) {
		gError() << "Error receiving packet from codec context";
		return;
	}

	if ( mRecorder and (int32_t)mRecorderTrackId != -1 ) {
		bool keyframe = packet.flags & AV_PKT_FLAG_KEY;
		mRecorder->WriteSample( mRecorderTrackId, timestamp_us, packet.data, packet.size, keyframe );
	}

	av_packet_unref(&packet);
}
