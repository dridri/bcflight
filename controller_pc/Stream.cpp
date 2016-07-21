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


Stream::Stream( QWidget* parent )
	: QGLWidget( parent )
	, mStreamThread( new StreamThread( this ) )
	, mLink( nullptr )
	, mShader( nullptr )
	, mFpsCounter( 0 )
	, mFps( 0 )
	, mFpsTimer( QElapsedTimer() )
{
	memset( &mY, 0, sizeof(mY) );
	memset( &mU, 0, sizeof(mU) );
	memset( &mV, 0, sizeof(mV) );

	mStreamThread->start();
	connect( this, SIGNAL( repaintEmitter() ), this, SLOT( repaintReceiver() ) );

	qDebug() << "WelsCreateDecoder :" << WelsCreateDecoder( &mDecoder );

	SDecodingParam decParam;
	memset( &decParam, 0, sizeof (SDecodingParam) );
	decParam.uiTargetDqLayer = UCHAR_MAX;
	decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
	decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;

	qDebug() << "mDecoder->Initialize :" << mDecoder->Initialize( &decParam );
}


Stream::~Stream()
{
	mDecoder->Uninitialize();
	WelsDestroyDecoder( mDecoder );
}


void Stream::setLink( Link* l )
{
	mLink = l;
}


int32_t Stream::fps()
{
	return mFps;
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
		mShader->addShaderFromSourceCode( QGLShader::Vertex,
			"attribute highp vec2 vertex;\n"
			"attribute highp vec2 tex;\n"
			"varying vec2 texcoords;\n"
			"void main(void) {\n"
			"	texcoords = vec2( tex.s, 1.0 - tex.t );\n"
			"	gl_Position = vec4( vertex, 0.0, 1.0 );\n"
			"}" );
		mShader->addShaderFromSourceCode( QGLShader::Fragment,
			"uniform sampler2D texY;\n"
			"uniform sampler2D texU;\n"
// 			"uniform sampler2D texV;\n"
			"varying vec2 texcoords;\n"
			"vec4 fetch( vec2 coords ) {\n"
			"	float y = texture2D( texY, coords ).r;\n"
			"	float u = texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.0) ).r - 0.5;\n"
			"	float v = texture2D( texU, coords * vec2(1.0, 0.5) + vec2(0.0, 0.5) ).r - 0.5;\n"
			"	float r = 2*(y/2 + 1.402/2 * v);\n"
			"	float g = 2*(y/2 - 0.344136 * u/2 - 0.714136 * v/2);\n"
			"	float b = 2*(y/2 + 1.773/2 * u);\n"
			"	return clamp( vec4( r, g, b, 1.0 ), 0.0, 1.0 );\n"
			"}\n"
			"void main(void) {\n"
			"	vec4 center = fetch( texcoords.st );\n"
			"	vec4 top = fetch( texcoords.st + vec2( 0.0, -0.0001 ) );\n"
			"	vec4 bottom = fetch( texcoords.st + vec2( 0.0, +0.0001 ) );\n"
			"	vec4 left = fetch( texcoords.st + vec2( -0.0001, 0.0 ) );\n"
			"	vec4 right = fetch( texcoords.st + vec2( +0.0001, 0.0 ) );\n"
			"	gl_FragColor = center * 5.0 - top - left - bottom - right;\n"
			"}" );
		mShader->link();
		mShader->bind();
	}
	if ( mY.tex == 0 and mU.tex == 0 and mV.tex == 0 and mY.stride > 0 and mY.height > 0 ) {
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
		 1.0f, -1.0f,  1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f,  0.0f, 1.0f,
		-1.0f,  1.0f,  0.0f, 0.0f,
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

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mY.tex );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mY.stride, mY.height, GL_RED, GL_UNSIGNED_BYTE, mY.data );
	glActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mU.tex );
// 	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mU.stride, mU.height, GL_RED, GL_UNSIGNED_BYTE, mU.data );
// 	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, mU.height, mV.stride, mV.height, GL_RED, GL_UNSIGNED_BYTE, mV.data );
	uint8_t d[1024*2048];
	memcpy( d, mU.data, mU.stride * mU.height );
	memcpy( d + mU.stride * mU.height, mV.data, mU.stride * mV.height );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mU.stride, mU.height + mV.height, GL_RED, GL_UNSIGNED_BYTE, d );
/*
	glActiveTexture( GL_TEXTURE2 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mV.tex );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, mV.stride, mV.height, GL_RED, GL_UNSIGNED_BYTE, mV.data );
*/
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

	uint8_t data[16384] = { 0 };
	int32_t size = mLink->Read( data, sizeof( data ), 0 );
	if ( size > 0 ) {
		DecodeFrame( data, size );
		if ( size > 19 ) {
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
// dsErrorFree;

void Stream::DecodeFrame( const uint8_t* src, size_t sliceSize )
{
	uint8_t* data[3];
	SBufferInfo bufInfo;

	memset( data, 0, sizeof(data) );
	memset( &bufInfo, 0, sizeof(SBufferInfo) );

	/*qDebug() << "mDecoder->DecodeFrame2 :" << */mDecoder->DecodeFrameNoDelay( src, (int)sliceSize, data, &bufInfo );

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
