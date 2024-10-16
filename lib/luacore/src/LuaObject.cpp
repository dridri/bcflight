#include "LuaObject.h"

LuaObject::LuaObject( Lua* state )
	: mState( state )
	, mReference( 0 )
{
	if ( lua_type( state->state(), -1 ) == LUA_TUSERDATA ) {
		lua_pushvalue( state->state(), -1 );
		mReference = luaL_ref( state->state(), LUA_REGISTRYINDEX );
		// gDebug() << "mReference : " << mReference;
	}
}


LuaObject::~LuaObject()
{
}
