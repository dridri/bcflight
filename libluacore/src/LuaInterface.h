#ifndef LUA_INTERFACE_H
#define LUA_INTERFACE_H

#include "Lua.h"


class LuaInterface : public LuaValue {
private:
	template< class C, size_t... S > static C* unpack_vector( const std::vector<LuaValue>& vec, std::index_sequence<S...> ) {
		return new C(vec[S]...);
	}

	template< class C, size_t size > static C* unpack_vector( const std::vector<LuaValue>& vec ) {
		return unpack_vector<C>( vec, std::make_index_sequence<size>() );
	}
public:
	template <class C, size_t nArgs> static void RegisterClass( Lua* lua, const string& name = typeid(LuaInterface).name() ) {
		lua_State* L = lua->state();

		lua_createtable( L, 0, 0 );

			lua_pushstring( L, "c-class" );
			lua_setfield( L, -2, "type" );

			lua_pushstring( L, name.c_str() );
			lua_setfield( L, -2, "name" );

			lua_pushcclosure( L, []( lua_State* L ) {
				lua_pushvalue( L, 1 );
				lua_setfield( L, -2, "super" );
				lua_pushvalue( L, 2 );
				lua_setfield( L, -2, "name" );
				// Create metatable with __index to access super-Class values
				lua_getmetatable( L, -1 );
					lua_pushvalue( L, 1 );
					lua_setfield( L, -2, "__index" );
				lua_pop( L, 1 );
				lua_pushvalue( L, -1 );
				return 1;
			}, 0 );
			lua_setfield( L, -2, "extend" );

			// Create metatable
			lua_createtable( L, 0, 0 );
				lua_pushcclosure( L, []( lua_State* L ) {
					int32_t n = lua_gettop( L );
					vector< LuaValue > args;
					for ( int32_t i = 1; i < n; i++ ) {
						args.push_back( Lua::value( L, i + 1 ) );
					}
					C** pInstance = reinterpret_cast<C**>( lua_newuserdata( L, sizeof(C*) ) );
					lua_pushvalue( L, -1 );
					*pInstance = unpack_vector< C, nArgs >( args );
					return 1;
				}, 0 );
				lua_setfield( L, -2, "__call" );
			lua_setmetatable( L, -2 );
		lua_setglobal( L, name.c_str() );
	}

protected:
	LuaInterface( Lua* lua, const string& className = typeid(LuaInterface).name() ) : mLua( lua ), mClassName( className ) {
		PreInit();
		CallMember( "init" );
	}
	template< typename ...LuaValues > LuaInterface( Lua* lua, const string& className, LuaValues... values ) : mLua( lua ), mClassName( className ) {
		PreInit();
		CallMember( "init", values... );
	}
	virtual ~LuaInterface() = 0;

	LuaValue CallMember( const string& funcname, const vector<LuaValue>& args );
	template< typename ...LuaValues > LuaValue CallMember( const string& funcname, LuaValues... values ) {
		return CallMember( funcname, { values... } );
	}
/*
	template< typename T > class Shared {
	public:
		Shared& operator=( const T& from ) {
			mValue = from;
			return *this;
		}
		operator T& () {
			return mValue;
		}
		operator const T& () {
			return mValue;
		}
	protected:
		T mValue;
	};
*/
	Lua* mLua;
	string mClassName;
// 	lua_Integer mInstanceRef;

private:
	void PreInit();
	std::map< std::string, LuaValue > mValues;
};


#endif // LUA_INTERFACE_H
