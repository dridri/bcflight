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
