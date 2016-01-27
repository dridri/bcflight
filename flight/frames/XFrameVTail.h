#ifndef XFRAMEVTAIL_H
#define XFRAMEVTAIL_H

#include <Vector.h>
#include "XFrame.h"

class XFrameVTail : public XFrame
{
public:
	XFrameVTail( Config* config );
	virtual ~XFrameVTail();

	virtual void Stabilize( const Vector3f& pid_output, const float& thrust );

	static Frame* Instanciate( Config* config );
	static int flight_register( Main* main );
};

#endif // XFRAMEVTAIL_H
