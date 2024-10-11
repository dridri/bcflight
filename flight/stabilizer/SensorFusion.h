#pragma once

#include <stdint.h>

template <typename V> class SensorFusion
{
public:
	virtual ~SensorFusion() {}
	virtual void UpdateInput( uint32_t row, const V& input ) = 0;
	virtual void Process( float dt ) = 0;
	virtual const V& state() const = 0;
};
