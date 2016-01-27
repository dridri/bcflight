#ifndef FRAME_H
#define FRAME_H

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <stdint.h>
#include <Motor.h>
#include <Vector.h>

class IMU;
class Controller;
class Config;

class Frame
{
public:
	Frame();
	virtual ~Frame();

	const std::vector< Motor* >& motors() const;

	virtual void Arm() = 0;
	virtual void Disarm() = 0;
	virtual void WarmUp() = 0;
	virtual void Stabilize( const Vector3f& pid_output, const float& thrust ) = 0;

	static Frame* Instanciate( const std::string& name, Config* config );
	static void RegisterFrame( const std::string& name, std::function< Frame* ( Config* ) > instanciate );
	static const std::map< std::string, std::function< Frame* ( Config* ) > > knownFrames() { return mKnownFrames; }

protected:
	std::vector< Motor* > mMotors;

private:
	static std::map< std::string, std::function< Frame* ( Config* ) > > mKnownFrames;
};

#endif // FRAME_H
