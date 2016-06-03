#ifndef CONTROLLERPC_H
#define CONTROLLERPC_H

#include <QtCore/QThread>
#include <Controller.h>

class ControllerPC : public Controller
{
public:
	ControllerPC( Link* link );
	~ControllerPC();

	void setControlThrust( const float v );

protected:
	float ReadThrust();
	float ReadRoll();
	float ReadPitch();
	float ReadYaw();
	int8_t ReadSwitch( uint32_t id );

private:
	float mThrust;
};


class ControllerMonitor : public QThread
{
	Q_OBJECT
public:
	ControllerMonitor( ControllerPC* controller );
	~ControllerMonitor();

signals:
	void connected();

protected:
	virtual void run();

private:
	ControllerPC* mController;
	bool mKnowConnected;
};

#endif // CONTROLLERPC_H
