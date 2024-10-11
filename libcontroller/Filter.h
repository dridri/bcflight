#pragma once

#include "Vector.h"

template<typename V> class Filter {
public:
	virtual ~Filter() {}

	virtual V filter( const V& input, float dt ) = 0;
	virtual V state() = 0;
};
