#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

class Camera
{
public:
	Camera();
	~Camera();

	virtual void StartRecording() = 0;
	virtual void StopRecording() = 0;

	virtual const uint32_t brightness() const = 0;
	virtual void setBrightness( uint32_t value ) = 0;
};


#endif // CAMERA_H
