#ifndef CONTROLLERPI_H
#define CONTROLLERPI_H

#include <Controller.h>
#include <Link.h>

class ControllerPi : public Controller
{
public:
	ControllerPi( Link* link );
	~ControllerPi();

protected:
	int8_t ReadSwitch( uint32_t id );
};

#endif // CONTROLLERPI_H
