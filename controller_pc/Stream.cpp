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

#include <unistd.h>
#include <GL/gl.h>
#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include "Stream.h"
#include "MainWindow.h"
#include "ControllerPC.h"
#include "ui_mainWindow.h"


#if ( defined(WIN32) && !defined(glActiveTexture) )
PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
#endif

Stream::Stream( QWidget* parent )
	: QGLWidget( parent )
	, mStreamThread( new StreamThread( this ) )
	, mAudioThread( new AudioThread( this ) )
	, mMainWindow( nullptr )
	, mLink( nullptr )
	, mAudioLink( nullptr )
	, mWidth( 0 )
	, mHeight( 0 )
	, mShader( nullptr )
	, mFpsCounter( 0 )
	, mFps( 0 )
	, mFpsTimer( QElapsedTimer() )
	, mParentWidget( parent )
{
	memset( &mY, 0, sizeof(mY) );
	memset( &mU, 0, sizeof(mU) );
	memset( &mV, 0, sizeof(mV) );

#ifdef WIN32
	if ( glActiveTexture == 0 ) {
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetProcAddress( LoadLibrary("opengl32.dll"), "glActiveTexture" );
	}
#endif

	mAudioDevice = QAudioDeviceInfo::defaultOutputDevice();
	mAudioFormat.setChannelCount( 1 );
	mAudioFormat.setCodec( "audio/pcm" );
	mAudioFormat.setSampleRate( 44100 );
	mAudioFormat.setByteOrder( QAudioFormat::LittleEndian );
	mAudioFormat.setSampleType( QAudioFormat::SignedInt );
	mAudioFormat.setSampleSize( 16 );
	mAudioOutput = new QAudioOutput( mAudioDevice, mAudioFormat );
	mAudioStream = mAudioOutput->start();

	mStreamThread->start();
	mAudioThread->start();
	mStreamThread->setPriority( QThread::TimeCriticalPriority );
	mAudioThread->setPriority( QThread::TimeCriticalPriority );
	connect( this, SIGNAL( repaintEmitter() ), this, SLOT( repaintReceiver() ) );

	qDebug() << "WelsCreateDecoder :" << WelsCreateDecoder( &mDecoder );
	SDecodingParam decParam;
	memset( &decParam, 0, sizeof (SDecodingParam) );
	decParam.uiTargetDqLayer = UCHAR_MAX;
	decParam.eEcActiveIdc = ERROR_CON_SLICE_MV_COPY_CROSS_IDR;//ERROR_CON_SLICE_COPY;
	decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
	qDebug() << "mDecoder->Initialize :" << mDecoder->Initialize( &decParam );
}


Stream::~Stream()
{
	mDecoder->Uninitialize();
	WelsDestroyDecoder( mDecoder );
}


void Stream::setMainWindow( MainWindow* win )
{
	mMainWindow = win;
}


void Stream::setLink( Link* l )
{
	mLink = l;
}


void Stream::setAudioLink( Link* l )
{
	mAudioLink = l;
}


int32_t Stream::fps()
{
	return mFps;
}


void Stream::mouseDoubleClickEvent( QMouseEvent * e )
{
	if ( e->button() == Qt::LeftButton ) {
		if ( !isFullScreen() ) {
			setParent( nullptr );
			showFullScreen();
			mMainWindow->getUi()->tab->repaint();
		} else {
			if ( mMainWindow ) {
				showNormal();
				mMainWindow->getUi()->video_container->layout()->addWidget( this );
			}
		}
	}
}


void Stream::closeEvent( QCloseEvent* e )
{
	if ( isFullScreen() and mMainWindow ) {
		showNormal();
		mMainWindow->getUi()->video_container->layout()->addWidget( this );
	}
	e->ignore();
}

/*
void Stream::paintEvent( QPaintEvent* ev )
{
	QPainter p( this );
// 	p.fillRect( QRect( 0, 0, width(), height() ), QBrush( QColor( 16, 16, 16 ) ) );
// 	mMutex.lock();
// 	p.drawImage( 0, 0, mImage );
// 	mMutex.unlock();

	if ( mY.width > 0 and mY.height > 0 and mY.data != 0 ) {
		for ( int y = 0; y < mY.height; y++ ) {
			for ( int x = 0; x < mY.width; x++ ) {
				const int xx = x >> 1;
				const int yy = y >> 1;
				const int Y = mY.data[y * mY.stride + x] - 16;
				const int U = mU. data[yy * mU.stride + xx] - 128;
				const int V = mV.data[yy * mV.stride + xx] - 128;
				const int r = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
				const int g = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
				const int b = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);
// 				p.setBrush( QBrush( QColor( r, g, b ) ) );
// 				p.drawPoint( x, y );
			}
		}
	}

}
*/


void Stream::paintGL()
{
	if ( not mShader ) {
		mShader = new QGLShaderProgram();
		mShader->addShaderFromSourceCode( QGLShader::Vertex, R"(
			attribute highp vec2 vertex;
			attribute highp vec2 tex;
			varying vec2 texcoords;
			void main(void) {
				texcoords = vec2( tex.s, 1.0 - tex.t );
				gl_Position = vec4( vertex, 0.0, 1.0 );
			})" );
		mShader->addShaderFromSourceCode( QGLShader::Fragment, R"(
			uniform sampler2D texY;
			uniform sampler2D texU;
// 			uniform sampler2D texV;
			uniform float exposure_value;
			uniform float gamma_compensation;
			varying vec2 texcoords;
			vec4 fetch( vec2 coords ) {
				float y = 2.0 * texture2D( texY, coords ).r;
				float u = 2.0 * (texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.0) ).r - 0.5);
				float v = 2.0 * (texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.5) ).r - 0.5);
				float r = (y/2.0 + 1.402/2.0 * v);
				float g = (y/2.0 - 0.344136 * u/2.0 - 0.714136 * v/2.0);
				float b = (y/2.0 + 1.773/2.0 * u);
				return clamp( vec4( r, g, b, 1.0 ), vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0) );
			}
			void main(void) {
				vec4 color = fetch( texcoords.st );
// 				color.rgb = vec3(1.0) - exp( -color.rgb * vec3(exposure_value, exposure_value, exposure_value) );
// 				color.rgb = pow( color.rgb, vec3( 1.0 / gamma_compensation, 1.0 / gamma_compensation, 1.0 / gamma_compensation ) );
				gl_FragColor = color;
				gl_FragColor.a = 1.0;
			})" );
		/*
		mShader->addShaderFromSourceCode( QGLShader::Fragment, R"(
			uniform sampler2D texY;
			uniform sampler2D texU;
// 			uniform sampler2D texV;
			uniform float exposure_value;
			uniform float gamma_compensation;
			varying vec2 texcoords;
			vec4 fetch( vec2 coords ) {
				float y = 2*texture2D( texY, coords ).r;
				float u = 2*(texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.0) ).r - 0.5);
				float v = 2*(texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.5) ).r - 0.5);
				float r = 2*(y/2 + 1.402/2 * v);
				float g = 2*(y/2 - 0.344136 * u/2 - 0.714136 * v/2);
				float b = 2*(y/2 + 1.773/2 * u);
				return clamp( vec4( r, g, b, 1.0 ), vec4(0.0), vec4(1.0) );
			}
			void main(void) {
				vec4 center = fetch( texcoords.st );
			//	vec4 top = fetch( texcoords.st + vec2( 0.0, -0.00001 ) );
			//	vec4 bottom = fetch( texcoords.st + vec2( 0.0, +0.00001 ) );
			//	vec4 left = fetch( texcoords.st + vec2( -0.00001, 0.0 ) );
			//	vec4 right = fetch( texcoords.st + vec2( +0.00001, 0.0 ) );
				gl_FragColor = center;// * 5.0 - top - left - bottom - right;
				gl_FragColor.rgb = vec3(1.0) - exp( -gl_FragColor.rgb * vec3(exposure_value) );
				gl_FragColor.rgb = pow( gl_FragColor.rgb, vec3( 1.0 / gamma_compensation ) );
			})" );
		*/
		mShader->link();
		mShader->bind();
		mExposureID = mShader->uniformLocation( "exposure_value" );
		mGammaID = mShader->uniformLocation( "gamma_compensation" );
	}
// 	if ( mY.tex == 0 and mU.tex == 0 and mV.tex == 0 and mY.stride > 0 and mY.height > 0 ) {
	if ( mY.tex == 0 or mU.tex == 0 or mY.stride != mWidth or mY.height != mHeight ) {
		if ( mY.tex ) {
			glDeleteTextures( 1, &mY.tex );
		}
		if ( mU.tex ) {
			glDeleteTextures( 1, &mU.tex );
		}
		mWidth = mY.stride;
		mHeight = mY.height;
		printf( "====> SIZE : %d x %d\n", mY.stride, mY.height );
		glGenTextures( 1, &mY.tex );
		glBindTexture( GL_TEXTURE_2D, mY.tex );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, mY.stride, mY.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glGenTextures( 1, &mU.tex );
		glBindTexture( GL_TEXTURE_2D, mU.tex );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, mU.stride, mU.height + mV.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

	glViewport( 0, 0, width(), height() );
	glClear( GL_COLOR_BUFFER_BIT );
	static float quadVertices[] = {
		// X, Y, U, V
		 1.0f,  1.0f,  1.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
	};
	quadVertices[2] = (float)mY.width / (float)mY.stride;
	quadVertices[6] = (float)mY.width / (float)mY.stride;
	mShader->bind();
	glBindTexture( GL_TEXTURE_2D, 0 );

	if ( not mY.data or not mU.data or not mV.data ) {
		return;
	}

	int vertexLocation = mShader->attributeLocation( "vertex" );
	int texLocation = mShader->attributeLocation( "tex" );
	mShader->enableAttributeArray( vertexLocation );
	mShader->enableAttributeArray( texLocation );
	mShader->setAttributeArray( vertexLocation, quadVertices, 2, sizeof(float)*4 );
	mShader->setAttributeArray( texLocation, quadVertices + 2, 2, sizeof(float)*4 );

	int loc_Y = mShader->uniformLocation( "texY" );
	int loc_U = mShader->uniformLocation( "texU" );
// 	int loc_V = mShader->uniformLocation( "texV" );
	mShader->setUniformValue( loc_Y, 0 );
	mShader->setUniformValue( loc_U, 1 );
// 	mShader->setUniformValue( loc_V, 2 );

	if ( mMainWindow and mMainWindow->controller() and mMainWindow->controller()->nightMode() ) {
		mShader->setUniformValue( mExposureID, 4.0f );
		mShader->setUniformValue( mGammaID, 2.5f );
	} else {
		mShader->setUniformValue( mExposureID, 1.0f );
		mShader->setUniformValue( mGammaID, 1.0f );
	}

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mY.tex );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mY.stride, mY.height, GL_RED, GL_UNSIGNED_BYTE, mY.data );
	glActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mU.tex );
	uint8_t* d = new uint8_t[ ( mU.stride * mU.height + mU.stride * mV.height ) * 2 ];
	memcpy( d, mU.data, mU.stride * mU.height );
	memcpy( d + mU.stride * mU.height, mV.data, mU.stride * mV.height );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mU.stride, mU.height + mV.height, GL_RED, GL_UNSIGNED_BYTE, d );
	delete d;

	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	mShader->disableAttributeArray( vertexLocation );
	mShader->disableAttributeArray( texLocation );
}


void Stream::repaintReceiver()
{
	repaint();
}


bool Stream::run()
{
	if ( not mLink ) {
		usleep( 1000 * 10 );
		return true;
	}
	if ( not mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			struct sched_param sched;
			memset( &sched, 0, sizeof(sched) );
			sched.sched_priority = sched_get_priority_max( SCHED_RR );
			sched_setscheduler( 0, SCHED_RR, &sched );
			mFpsTimer.start();
// 			uint32_t uid = htonl( 0x12345678 );
// 			mLink->Write( &uid, sizeof(uid), 0 );
		} else {
			usleep( 1000 * 500 );
		}
		return true;
	}

	uint8_t data[65536] = { 0 };
	int32_t size = mLink->Read( data, sizeof( data ), 0 );
	if ( size > 0 ) {
		DecodeFrame( data, size );
		if ( size > 41 ) {
			mFpsCounter++;
		}
	}

	if ( mFpsTimer.elapsed() >= 1000 ) {
		mFps = mFpsCounter;
		mFpsCounter = 0;
		mFpsTimer.restart();
	}

	return true;
}


bool Stream::runAudio()
{
	if ( not mAudioLink ) {
		usleep( 1000 * 10 );
		return true;
	}
	if ( not mAudioLink->isConnected() ) {
		mAudioLink->Connect();
		if ( mAudioLink->isConnected() ) {
			struct sched_param sched;
			memset( &sched, 0, sizeof(sched) );
			sched.sched_priority = sched_get_priority_max( SCHED_RR );
			sched_setscheduler( 0, SCHED_RR, &sched );
		} else {
			usleep( 1000 * 500 );
		}
		return true;
	}

	uint8_t data[65536] = { 0 };
	int32_t size = mAudioLink->Read( data, sizeof( data ), 0 );
	if ( size > 0 and mAudioStream != nullptr ) {
		mAudioStream->write( (const char*)data, size );
	}

	return true;
}


void Stream::DecodeFrame( const uint8_t* src, size_t sliceSize )
{
	uint8_t* data[3];
	SBufferInfo bufInfo;

	memset( data, 0, sizeof(data) );
	memset( &bufInfo, 0, sizeof(SBufferInfo) );

	DECODING_STATE ret = mDecoder->DecodeFrameNoDelay( src, (int)sliceSize, data, &bufInfo );
// 	printf( "        DecodeFrameNoDelay returned %08X\n", ret );

	if ( bufInfo.iBufferStatus == 1 ) {
		mY.stride = bufInfo.UsrData.sSystemBuffer.iStride[0];
		mY.width = bufInfo.UsrData.sSystemBuffer.iWidth;
		mY.height = bufInfo.UsrData.sSystemBuffer.iHeight;
		mY.data = data[0];

		mU.stride = bufInfo.UsrData.sSystemBuffer.iStride[1];
		mU.width = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		mU.height = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		mU.data = data[1];

		mV.stride = bufInfo.UsrData.sSystemBuffer.iStride[2];
		mV.width = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		mV.height = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		mV.data = data[2];
/*
		QImage image( mY.width, mY.height, QImage::Format_RGB32 );
		for ( int y = 0; y < mY.height; y++ ) {
			for ( int x = 0; x < mY.width; x++ ) {
				const int xx = x >> 1;
				const int yy = y >> 1;
				const int Y = data[0][y * bufInfo.UsrData.sSystemBuffer.iStride[0] + x] - 16;
				const int U = data[1][yy * bufInfo.UsrData.sSystemBuffer.iStride[1] + xx] - 128;
				const int V = data[2][yy * bufInfo.UsrData.sSystemBuffer.iStride[2] + xx] - 128;
				const int r = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
				const int g = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
				const int b = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);
				image.setPixel( x, y, QColor( r, g, b ).rgb() );
			}
		}

		mMutex.lock();
		mImage = image;
		mMutex.unlock();
*/
		emit repaintEmitter();
	}
}
