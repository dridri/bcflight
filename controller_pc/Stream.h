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
#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QtGui/QPaintEvent>
#include <QtGui/QImage>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioFormat>

#include <Link.h>
extern "C" {
#include "../../external/openh264-master/codec/api/svc/codec_api.h"
}

class MainWindow;

class Stream : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	Stream( QWidget* parent );
	virtual ~Stream();
	void setMainWindow( MainWindow* win );
	void setLink( Link* l );
	void setAudioLink( Link* l );
	int32_t fps();

protected:
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void mouseDoubleClickEvent( QMouseEvent * e );
	virtual void closeEvent( QCloseEvent* e );
	bool run();
	bool runAudio();
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
	class AudioThread : public QThread
	{
	public:
		AudioThread( Stream* s ) : mStream( s ) {}
	protected:
		virtual void run() { while ( mStream->runAudio() ); }
		Stream* mStream;
	};

	StreamThread* mStreamThread;
	AudioThread* mAudioThread;
	MainWindow* mMainWindow;
	Link* mLink;
	Link* mAudioLink;
	uint32_t mSocketTellIPCounter;
	ISVCDecoder* mDecoder;
	uint32_t mWidth;
	uint32_t mHeight;
	Plane mY;
	Plane mU;
	Plane mV;
	QImage mImage;
	QMutex mMutex;
	QOpenGLShaderProgram* mShader;
	uint32_t mFpsCounter;
	uint32_t mFps;
	QElapsedTimer mFpsTimer;
	QWidget* mParentWidget;
	uint32_t mExposureID;
	uint32_t mGammaID;
	QAudioOutput* mAudioOutput;
	QIODevice* mAudioStream;
	QAudioFormat mAudioFormat;
	QAudioDeviceInfo mAudioDevice;
};

#endif // STREAM_H
