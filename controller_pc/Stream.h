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

#ifndef STREAM_H
#define STREAM_H

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <QtGui/QPaintEvent>
#include <QtGui/QImage>

#include <Link.h>
#include <codec_api.h>

class Stream : public QGLWidget
{
	Q_OBJECT

public:
	Stream( QWidget* parent );
	virtual ~Stream();
	void setLink( Link* l );
	int32_t fps();

protected:
	virtual void paintGL();
	bool run();
	void DecodeFrame( const uint8_t* src, size_t sliceSize );

signals:
	void repaintEmitter();

protected slots:
	void repaintReceiver();

private:
	class StreamThread : public QThread
	{
	public:
		StreamThread( Stream* s ) : mStream( s ) {}
	protected:
		virtual void run() { while ( mStream->run() ); }
		Stream* mStream;
	};
	struct Plane {
		uint32_t stride;
		uint32_t width;
		uint32_t height;
		uint8_t* data;
		GLuint tex;
	};

	StreamThread* mStreamThread;
	Link* mLink;
	ISVCDecoder* mDecoder;
	Plane mY;
	Plane mU;
	Plane mV;
	QImage mImage;
	QMutex mMutex;
	QGLShaderProgram* mShader;
	uint32_t mFpsCounter;
	uint32_t mFps;
	QElapsedTimer mFpsTimer;
};

#endif // STREAM_H
