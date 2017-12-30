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
#include "VideoViewer.h"


#if ( defined(WIN32) && !defined(glActiveTexture) )
PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
#endif

VideoViewer::VideoViewer( QWidget* parent )
	: QGLWidget( parent )
	, mWidth( 0 )
	, mHeight( 0 )
	, mShader( nullptr )
	, mFpsCounter( 0 )
	, mFps( 0 )
	, mFpsTimer( QElapsedTimer() )
	, mParentWidget( parent )
	, mVibrance( 0.0f )
	, mTemperature( 0.0f )
{
	memset( &mY, 0, sizeof(mY) );
	memset( &mU, 0, sizeof(mU) );
	memset( &mV, 0, sizeof(mV) );

#ifdef WIN32
	if ( glActiveTexture == 0 ) {
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetProcAddress( LoadLibrary("opengl32.dll"), "glActiveTexture" );
	}
#endif

	connect( this, SIGNAL( repaintEmitter() ), this, SLOT( repaintReceiver() ) );
}


VideoViewer::~VideoViewer()
{
}


VideoViewer::Plane& VideoViewer::planeY()
{
	return mY;
}


VideoViewer::Plane& VideoViewer::planeU()
{
	return mU;
}


VideoViewer::Plane& VideoViewer::planeV()
{
	return mV;
}


int32_t VideoViewer::fps()
{
	return mFps;
}


void VideoViewer::invalidate()
{
	emit repaintEmitter();
}


void VideoViewer::repaintReceiver()
{
	repaint();
}


void VideoViewer::paintGL()
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
			uniform float temperature;
			uniform float vibrance;
			varying vec2 texcoords;
			vec3 rgb2hsv(vec3 c) {
				vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
				vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
				vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

				float d = q.x - min(q.w, q.y);
				float e = 1.0e-10;
				return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
			}

			vec3 hsv2rgb(vec3 c) {
				vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
				vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
				return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
			}
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
				color.rgb = vec3(1.0) - exp( -color.rgb * vec3(exposure_value, exposure_value, exposure_value) );
				color.rgb = pow( color.rgb, vec3( 1.0 / gamma_compensation, 1.0 / gamma_compensation, 1.0 / gamma_compensation ) );
				vec3 hsv = rgb2hsv( color.rgb );
				hsv.y *= vibrance;
				color.rgb = hsv2rgb( hsv );
				gl_FragColor = color;
				gl_FragColor.a = 1.0;
			})" );
		mShader->link();
		mShader->bind();
		mExposureID = mShader->uniformLocation( "exposure_value" );
		mGammaID = mShader->uniformLocation( "gamma_compensation" );
		mTemperatureID = mShader->uniformLocation( "temperature" );
		mVibranceID = mShader->uniformLocation( "vibrance" );
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

	// night mode
// 		mShader->setUniformValue( mExposureID, 4.0f );
// 		mShader->setUniformValue( mGammaID, 2.5f );
	mShader->setUniformValue( mExposureID, 1.0f );
	mShader->setUniformValue( mGammaID, 1.0f );
	mShader->setUniformValue( mTemperatureID, mTemperature );
	mShader->setUniformValue( mVibranceID, mVibrance );

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
	delete[] d;

	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	mShader->disableAttributeArray( vertexLocation );
	mShader->disableAttributeArray( texLocation );
}
