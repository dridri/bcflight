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

#ifndef VIDEOVIEWER_H
#define VIDEOVIEWER_H

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLShaderProgram>
#include <QtGui/QPaintEvent>
#include <QtGui/QImage>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioFormat>

#include <Link.h>
extern "C" {
#include "../../external/openh264-master/codec/api/wels/codec_api.h"
}

class MainWindow;

class VideoViewer : public QGLWidget
{
	Q_OBJECT

public:
	typedef struct {
		uint32_t stride;
		uint32_t width;
		uint32_t height;
		uint8_t* data;
		GLuint tex;
	} Plane;

	VideoViewer( QWidget* parent );
	virtual ~VideoViewer();

	Plane& planeY();
	Plane& planeU();
	Plane& planeV();

	void setVibrance( float f ) { mVibrance = f; }
	void setWhiteBalanceTemperature( float f ) { mTemperature = f; }
	void invalidate();
	int32_t fps();

protected:
	virtual void paintGL();

signals:
	void repaintEmitter();

protected slots:
	void repaintReceiver();

private:
	uint32_t mWidth;
	uint32_t mHeight;
	Plane mY;
	Plane mU;
	Plane mV;
	QImage mImage;
	QMutex mMutex;
	QGLShaderProgram* mShader;
	uint32_t mFpsCounter;
	uint32_t mFps;
	QElapsedTimer mFpsTimer;
	QWidget* mParentWidget;
	uint32_t mExposureID;
	uint32_t mGammaID;
	uint32_t mTemperatureID;
	uint32_t mVibranceID;
	float mVibrance;
	float mTemperature;
};

#endif // VIDEOVIEWER_H
