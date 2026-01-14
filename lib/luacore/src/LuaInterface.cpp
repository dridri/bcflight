#include "LuaInterface.h"


void LuaInterface::PreInit()
{
	lua_State* L = mLua->state();
	*reinterpret_cast<LuaInterface**>( lua_newuserdata( L, sizeof(LuaInterface*) ) ) = this;

	lua_pushvalue( L, -1 );
// 	mInstanceRef = luaL_ref( L, LUA_REGISTRYINDEX );
	mType = Reference;
	mReference[L] = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_createtable( L, 0, 0 );
		lua_createtable( L, 0, 0 );
			lua_createtable( L, 0, 0 );
				lua_getglobal( L, mClassName.c_str() );
					lua_getfield( L, -1, "__index" );
					lua_setfield( L, -3, "__index" );
				lua_pop( L, 1 );
			lua_setmetatable( L, -2 );
		lua_setfield( L, -2, "__index" );
		lua_getfield( L, -1, "__index" );
		lua_setfield( L, -2, "__newindex" );
	lua_setmetatable( L, -2 );
}


LuaInterface::~LuaInterface()
{
}


LuaValue LuaInterface::CallMember( const string& funcname, const vector<LuaValue>& args )
{
	lua_State* L = mLua->state();

	mLua->mutex().lock();
	int nArgs = args.size() + 1;

	lua_getglobal( L, "call_error" );
	int lua_top_base = lua_gettop( L );

	lua_rawgeti( L, LUA_REGISTRYINDEX, mReference[L] );
	lua_getfield( L, -1, funcname.c_str() );

	lua_pushvalue( L, -2 );
	for( const LuaValue& v : args ) {
		v.push( L );
	}

	LuaValue value_ret;
	int ret = lua_pcall( L, nArgs, LUA_MULTRET, -3 - nArgs );
	if ( ret != 0 ) {
		std::cout << "Lua error : " << lua_tostring( L, -1 );
		lua_pop( L, -1 );
	} else {
		int32_t nRets = ( lua_gettop( L ) - 1 ) - lua_top_base;
		if ( nRets == 1 ) {
			value_ret = mLua->value( -1 );
		} else if ( nRets > 1 ) {
			for ( int32_t i = 0; i < nRets; i++ ) {
				value_ret[nRets - i - 1] = mLua->value( -1 - i );
			}
		}
		lua_pop( L, nRets );
	}

	lua_pop( L, 1 ); // closing lua_rawgeti
	mLua->mutex().unlock();
	return value_ret;
}
