#pragma once

#include <Vector.h>
#include <Lua.h>
#include <Debug.h>
#include "Filter.h"

template<typename V> class FilterChain : public Filter<V>
{
public:
	FilterChain() : mState( V() ) {
		fDebug();
	}
	FilterChain( const std::list<Filter<V>*>& filters ) : mFilters( filters ), mState( V() ) {
		fDebug( mFilters.size() );
	}

	virtual V filter( const V& input, float dt ) {
		V value = input;
		for ( auto f : mFilters ) {
			value = f->filter( value, dt );
		}
		mState = value;
		return value;
	}
	virtual V state() {
		return mState;
	}

	const std::list< Filter<V>* >& filters() const { return mFilters; }

protected:
	std::list< Filter<V>* > mFilters;
	V mState;
};

LUA_CLASS class FilterChain_1 : public FilterChain<float>
{
public:
	LUA_EXPORT FilterChain_1( const LuaValue& filters ) : FilterChain( filters ) {}
};

LUA_CLASS class FilterChain_2 : public FilterChain<Vector2f>
{
public:
	LUA_EXPORT FilterChain_2( const LuaValue& filters ) : FilterChain( filters ) {}
};

LUA_CLASS class FilterChain_3 : public FilterChain<Vector3f>
{
public:
	LUA_EXPORT FilterChain_3( const LuaValue& filters ) : FilterChain( filters ) {}
};
