#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>
#include <netdb.h>
#include <netinet/in.h>
#include "Lua.h"


map< lua_State*, Lua* > Lua::mStates;
LuaValue LuaValue::mNil;

LuaValue LuaValue::Value( Lua* state, int32_t index )
{
	lua_State* L = state->state();
	LuaValue ret;
	lua_pushvalue( L, index );
	int32_t ref = luaL_ref( L, LUA_REGISTRYINDEX );
	ret.mType = Reference;
	ret.mReference[L] = index;
	return ret;
}

LuaValue LuaValue::Registry( Lua* state, int32_t index )
{
	LuaValue ret;
	ret.mType = Reference;
	ret.mReference[state->state()] = index;
	return ret;
}


LuaValue LuaValue::fromReference( Lua* lua, lua_Integer ref )
{
	LuaValue v;
	v.mType = Reference;
	v.mReference[lua->state()] = ref;
	return v;
}


LuaValue LuaValue::fromReference( Lua* lua, LuaValue& val )
{
	LuaValue v;
	v.mType = Reference;
	v.mReference[lua->state()] = val.mReference[lua->state()];
	return v;
}


LuaValue LuaValue::fromReference( Lua* lua, LuaValue* val )
{
	LuaValue v;
	v.mType = Reference;
	v.mReference[lua->state()] = val->mReference[lua->state()];
	return v;
}


static int LuaPrint( lua_State* L )
{
	int nArgs = lua_gettop(L);
	lua_getglobal( L, "tostring" );
	string ret = "";
	for ( int i = 1; i <= nArgs; i++ ) {
		lua_pushvalue( L, -1 );
		lua_pushvalue( L, i );
		lua_call( L, 1, 1 );
		const char* s = lua_tostring( L, -1 );
		if ( s == nullptr ) {
			break;
		}
		if ( i > 1 ) {
			ret += "\t";
		}
		ret += s;
		lua_pop( L, 1 );
	};
	std::cout << "\x1B[93m[Lua]\x1B[0m " << ret << "\n";
	return 0;
}

static int LuaDebugPrint( lua_State* L )
{
	int nArgs = lua_gettop(L);
	lua_getglobal( L, "tostring" );
	// Debug::Level level = static_cast<Debug::Level>( lua_tointeger( L, 1 ) );
	string ret = "";
	for ( int i = 2; i <= nArgs; i++ ) {
		lua_pushvalue( L, -1 );
		lua_pushvalue( L, i );
		lua_call( L, 1, 1 );
		const char* s = lua_tostring( L, -1 );
		if ( s == nullptr ) {
			break;
		}
		if ( i > 2 ) {
			ret += "\t";
		}
		ret += s;
		lua_pop( L, 1 );
	};
	std::cout << "\x1B[93m[Lua]\x1B[0m " << ret;
	return 0;
}


static int LuaDoFile( lua_State* L )
{
	int nArgs = lua_gettop(L);
	if ( nArgs == 0 ) {
		return -1;
	}

	const char* filename = lua_tostring( L, 1 );

	string path = "";
	lua_Debug entry;
	int depth = 1;
	while ( lua_getstack(L, depth, &entry) )
	{
		lua_getinfo(L, "Slnu", &entry);
// 		printf("%s(%d): %s\n", entry.source + 1, entry.currentline, entry.name ? entry.name : "?");
		if ( entry.name and string(entry.name) == "loadtable" ) {
			depth++;
			continue;
		}
		string source = entry.source + 1;
		transform( source.begin(), source.end(), source.begin(), ::tolower );
// 		std::cout << "source : " << source;
		if ( source.find("/") != source.npos or source.find(".lua") != source.npos ) {
			path = entry.source + 1;
			auto end = path.rfind( "/" );
			if ( end != path.npos ) {
				path = path.substr( 0, end );
			}
			if ( path.rfind( "/" ) == path.npos ) {
				path = ".";
			}
			break;
		}
		depth++;
	}

	return Lua::states()[L]->do_file( filename, path );
}


static uint32_t crc32_for_byte( uint32_t r ) {
	for( int j = 0; j < 8; ++j ) {
		r = ( r & 1 ? 0: (uint32_t)0xEDB88320L ) ^ r >> 1;
	}
	return r ^ (uint32_t)0xFF000000L;
}


static int LuaCRC32( lua_State* L )
{
	static uint32_t table[0x100] = { 0 };

	if( !*table ) {
		for ( size_t i = 0; i < 0x100; ++i ) {
			table[i] = crc32_for_byte(i);
		}
	}

	size_t len = 0;
	uint8_t* data = (uint8_t*)lua_tolstring( L, 1, &len );
	uint32_t crc = 0;

	for ( size_t i = 0; i < len; ++i ) {
		crc = table[ (uint8_t)crc ^ data[i] ] ^ crc >> 8;
	}

	lua_pushinteger( L, crc );
	return 1;
}


static int LuaLoadFile( lua_State* L )
{
	int nArgs = lua_gettop(L);
	if ( nArgs == 0 ) {
		return -1;
	}

	const char* filename_ = lua_tostring( L, 1 );

	string path = "";
	lua_Debug entry;
	int depth = 1;
	while ( lua_getstack(L, depth, &entry) )
	{
		lua_getinfo(L, "Sln", &entry);
// 		printf("%s(%d): %s\n", entry.source + 1, entry.currentline, entry.name ? entry.name : "?");
		if ( entry.name and string(entry.name) == "loadtable" ) {
			depth++;
			continue;
		}
		string source = entry.source + 1;
		transform( source.begin(), source.end(), source.begin(), ::tolower );
// 		std::cout << "source : " << source;
		if ( source.find("/") != source.npos or source.find(".lua") != source.npos ) {
			path = entry.source + 1;
			auto end = path.rfind( "/" );
			if ( end != path.npos ) {
				path = path.substr( 0, end );
			}
			if ( path.rfind( "/" ) == path.npos ) {
				path = ".";
			}
			break;
		}
		depth++;
	}

	int lua_top_base = lua_gettop( L );
	string filename = filename_;

	if ( access( filename.c_str(), R_OK ) == -1 and filename[0] != '/' ) {
		if ( access( ( path + "/" + filename ).c_str(), R_OK ) == 0 ) {
			filename = path + "/" + filename;
		} else {
			filename = string("modules/") + filename;
		}
	}

	if ( access( filename.c_str(), R_OK ) == -1 ) {
		std::cout << "ERROR : " << "Cannot open file " << filename_ << " : " << strerror(errno);
		return -1;
	}
	int32_t ret = luaL_loadfilex( L, filename.c_str(), nullptr );
// 	printf("ret : %d\n", ret );
	if ( ret != 0 ) {
		std::cout << "ERROR : " << lua_tostring( L, -1 );
		return -1;
	}
	return 1;
}


int Lua::Traceback( lua_State* L )
{
	Lua* state = Lua::states()[L];

	string trace_first = state->lastCallName();
	auto print = [L, trace_first]( int depth, lua_Debug& entry ) {
		lua_getinfo( L, "Slnu", &entry );
		string name = ( entry.name ? entry.name : ( trace_first == "" ? "??" : trace_first ) );
		string args = "";
#ifdef LUAJIT_VERSION
#else
		for ( uint32_t i = 0; i < entry.nparams; i++ ) {
			args += string(i == 0 ? "" : ", ") + lua_getlocal( L, &entry, i + 1 );
		}
#endif
		return string("    in ") + name + "(" + args + ") at " + entry.short_src + ":" + to_string(entry.currentline);
	};

	std::cout << "ERROR : " << "Stack trace :";

	lua_Debug entry;
	int depth = 1;
	while ( lua_getstack( L, depth, &entry ) )
	{
		std::string line = print( depth, entry );
		state->mLastError += line + "\n";
		std::cout << "ERROR : " << line << "\n";
		depth++;
	}

	// We did not unroll anything, so we print the very first entry
	if ( depth == 1 ) {
		int ret = lua_getstack( L, 1, &entry );
		if ( ret == 1 ) {
			std::string line = print( 1, entry );
			state->mLastError += line + "\n";
			std::cout << "ERROR : " << line << "\n";
		}
	}

	if ( Lua::states()[L]->exitOnError() ) {
		exit(0);
	}

	return 0;
}


int Lua::CallError( lua_State* L )
{
	// fDebug();
	Lua* state = Lua::states()[L];
	const char* message = lua_tostring( L, 1 );
	std::cout << "ERROR : " << message << "\n";
	state->mLastError += string(message) + "\n";
	Lua::Traceback( L );
	return 1;
}


LuaValue::FunctionRef::~FunctionRef()
{
// 	std::cout << "lua-function unref( " << mLua << ", LUA_REGISTRYINDEX, " << mRef << " )";
// 	mLua->mutex().lock();
	luaL_unref( mLua->state(), LUA_REGISTRYINDEX, mRef );
// 	mLua->mutex().unlock();
}


void LuaValue::FunctionRef::Unref()
{
	mCounter--;
	if ( mCounter == 0 ) {
		delete this;
	}
}


LuaValue LuaValue::FunctionRef::Call( const vector<LuaValue>& args )
{
	return mLua->CallFunction( mRef, args );
}


Lua::Lua( const string& load_path )
	: mExitOnError( false )
{
	// fDebug( load_path );

	mLuaState = luaL_newstate();
	std::cout << "state : " << mLuaState;
	mStates.emplace( make_pair( mLuaState, this ) );
	luaL_openlibs( mLuaState );

	lua_register( mLuaState, "print", &LuaPrint );
	lua_register( mLuaState, "debugprint", &LuaDebugPrint );
	lua_register( mLuaState, "dofile", &LuaDoFile );
	lua_register( mLuaState, "loadfile", &LuaLoadFile );
	lua_register( mLuaState, "traceback", &Traceback );
	lua_register( mLuaState, "call_error", &CallError );
	lua_register( mLuaState, "crc32", &LuaCRC32 );
	lua_register( mLuaState, "serialize", []( lua_State* L ) {
		LuaValue v = Lua::value( L, 1 );
		LuaValue( v.serialize() ).push( L );
		return 1;
	});

	do_string( "package.path = \"lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"../lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"../lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"../lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"../libs/libluacore/lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"../libs/libluacore/lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"../libs/libluacore/lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../libs/libluacore/lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../libs/libluacore/lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../libs/libluacore/lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../../libs/libluacore/lua/?.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../../libs/libluacore/lua/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../../libs/libluacore/lua/?/?/init.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/modules/?.lua;\" .. package.path" );
	do_string( "package.path = \"" + load_path + "/../modules/?.lua;\" .. package.path" );
	do_string( "package.path = \"modules/?.lua;\" .. package.path" );
	do_string( "package.path = \"../modules/?.lua;\" .. package.path" );
	if ( getenv("HOME") ) {
		do_string( std::string("package.path = \"") + getenv("HOME") + "/.luarocks/share/lua/5.1/?.lua;\" .. package.path" );
		do_string( std::string("package.path = \"") + getenv("HOME") + "/.luarocks/share/lua/5.1/?/init.lua;\" .. package.path" );
		do_string( std::string("package.path = \"") + getenv("HOME") + "/.luarocks/share/lua/5.1/?/?/init.lua;\" .. package.path" );
		do_string( std::string("package.cpath = \"") + getenv("HOME") + "/.luarocks/lib/lua/5.1/?.so;\" .. package.cpath" );
	}

	RegisterMember( "os", "sleep", []( const LuaValue& t ) {
		usleep( ( t.toNumber() * 1000.0f ) * 1000LLU );
		return LuaValue();
	});

	RegisterMember( "io", "readFile", [this]( const LuaValue& filename ) {
		FILE* fp = fopen64( filename.toString().c_str(), "rb" );
		if ( fp ) {
			fseeko64( fp, 0, SEEK_END );
			size_t size = ftello64( fp );
			fseeko64( fp, 0, SEEK_SET );
			uint8_t* buf = new uint8_t[size];
			fread( buf, 1, size, fp );
			LuaValue ret = LuaValue( (const char*)buf, size );
			delete[] buf;
			fclose( fp );
			return ret;
		}
		return LuaValue();
	});
	RegisterMember( "string", "lower", [this]( const LuaValue& v ) {
		string s1 = v.toString();
		string s2 = "";
		unsigned char* data = (unsigned char*)s1.data();
		for ( uint32_t i = 0; i < s1.length(); i++ ) {
			if ( data[i] >= 'A' and data[i] <= 'Z' ) {
				s2 += ( data[i] + 'a' - 'A' );
			} else if ( data[i] == 0xC3 and data[i + 1] >= 0x80 and data[i + 1] <= 0x9F ) {
				s2 += 0xC3;
				s2 += data[i + 1] + 32;
				i++;
			} else {
				s2 += data[i];
			}
		}
		return LuaValue( s2 );
	});
	RegisterMember( "string", "upper", [this]( const LuaValue& v ) {
		string s1 = v.toString();
		string s2 = "";
		unsigned char* data = (unsigned char*)s1.data();
		for ( uint32_t i = 0; i < s1.length(); i++ ) {
			if ( data[i] >= 'a' and data[i] <= 'z' ) {
				s2 += ( data[i] + 'A' - 'a' );
			} else if ( data[i] == 0xC3 and data[i + 1] >= 0xA0 and data[i + 1] <= 0xBF ) {
				s2 += 0xC3;
				s2 += data[i + 1] - 32;
				i++;
			} else {
				s2 += data[i];
			}
		}
		return LuaValue( s2 );
	});

	Reload();
}


Lua::~Lua()
{
}


void Lua::Reload()
{
}


vector<lua_Debug> Lua::trace()
{
	vector<lua_Debug> ret;

	lua_Debug entry;
	int depth = 0;
	while ( lua_getstack( mLuaState, depth, &entry ) )
	{
		lua_getinfo( mLuaState, "nSlu", &entry );
		ret.emplace_back( entry );
		depth++;
	}

	return ret;
}


int32_t Lua::do_file( const string& filename_, const string& path_tip )
{
	// fDebug( filename_, path_tip );
// 	mMutex.lock();
	string filename = filename_;

	if ( access( filename.c_str(), R_OK ) == -1 and filename[0] != '/' ) {
		list<string> pathTips = mPathTips;
		pathTips.push_back( path_tip );
		bool found = false;
		for ( string tip : pathTips ) {
			std::cout << "Testing with '" << ( tip + "/" + filename ) << "' and '" << ( tip + filename ) << "'\n";
			if ( access( ( tip + "/" + filename ).c_str(), R_OK ) == 0 ) {
				filename = tip + "/" + filename;
				found = true;
				break;
			} else if ( access( ( tip + filename ).c_str(), R_OK ) == 0 ) {
				filename = tip + filename;
				found = true;
				break;
			}
		}
		if ( not found ) {
			filename = string("modules/") + filename;
		}
	}

	lua_getglobal( mLuaState, "call_error" );
	int lua_top_base = lua_gettop( mLuaState );
	// fDebug( filename_, path_tip, " => " + filename );
	if ( access( filename.c_str(), R_OK ) == -1 ) {
		std::cout << "ERROR : " << "Cannot open file " << filename_ << " : " << strerror(errno) << "\n";
		mMutex.unlock();
		return -1;
	}
// 	std::cout << "luaL_loadfilex( mLuaState, " << filename << ", nullptr )";
	int32_t ret = luaL_loadfilex( mLuaState, filename.c_str(), nullptr );
	if ( ret != 0 ) {
		std::cout << "ERROR : " << lua_tostring( mLuaState, -1 ) << "\n";
		mMutex.unlock();
		return -1;
	}
	mLastCallName = "";
	ret = lua_pcall( mLuaState, 0, LUA_MULTRET, -2 );
	if ( ret != 0 ) {
		std::cout << "ERROR : " << lua_tostring( mLuaState, -1 ) << "\n";
		mMutex.unlock();
		return -1;
	} else {
		if ( mOpenedFiles.count( filename ) == 0 ) {
			mOpenedFiles.emplace( filename );
		}
		int32_t ret = lua_gettop( mLuaState ) - lua_top_base;
		mMutex.unlock();
		return ret;
	}
// 	mMutex.unlock();
	return ret;

/*
	string filename = filename_;

	if ( access( filename.c_str(), F_OK ) == -1 and filename[0] != '/' ) {
		filename = string("modules/") + filename;
	}

	luaL_loadfilex( mLuaState, filename.c_str(), nullptr );
	int32_t ret = lua_pcall( mLuaState, 0, LUA_MULTRET, 0 );
	if ( ret != 0 ) {
		std::cout << "ERROR : " << lua_tostring( mLuaState, -1 );
		return -1;
	}
	return ret;
*/
}


int32_t Lua::do_string( const string& str )
{
	mMutex.lock();
	lua_getglobal( mLuaState, "call_error" );
	luaL_loadstring( mLuaState, str.c_str() );
	mLastCallName = "";
	int32_t ret = lua_pcall( mLuaState, 0, 0, -2 );
	mMutex.unlock();
	return ret;
}


LuaValue Lua::value( lua_State* L, int index )
{
	int type = lua_type( L, index );
	if ( type == LUA_TNUMBER and fmod((float)lua_tonumber(L, index), 1.0f) == 0.0f ) {
		return LuaValue( lua_tointeger( L, index ) );
	} else if ( type == LUA_TNUMBER ) {
		return lua_tonumber( L, index );
	} else if ( type == LUA_TBOOLEAN ) {
		return LuaValue::boolean( lua_toboolean( L, index ) );
	} else if ( type == LUA_TSTRING ) {
		size_t len = 0;
		auto ret = lua_tolstring( L, index, &len );
		return LuaValue( ret, len );
	} else if ( type == LUA_TFUNCTION ) {
// 		mMutex.lock();
		LuaValue ret;
		lua_pushvalue( L, index );
		ret.setFunctionRef( mStates[L], luaL_ref( L, LUA_REGISTRYINDEX ) );
// 		mMutex.unlock();
		return ret;
	} else if ( lua_iscfunction( L, index ) ) {
	} else if ( type == LUA_TUSERDATA ) {
		LuaValue ret = lua_touserdata( L, index );
		return ret;
	} else if ( type == LUA_TTABLE ) {
		LuaValue table = LuaValue(std::map<string, string>());
		size_t len = lua_objlen( L, index );
		if ( len > 0 ) {
			for ( size_t i = 1; i <= len; i++ ) {
				lua_rawgeti( L, index, i );
				if ( not lua_isfunction( L, -1 ) ) {
					table[i] = value( L, -1 );
				}
				lua_pop( L, 1 );
			}
		} else {
			bool istop = ( index > 0 );
			if ( istop ) {
				lua_pushvalue( L, index );
			}
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				const char* key_ = lua_tostring( L, -2 );
				if ( key_ ) {
					std::string key = key_;
// 					bool dont_pop = lua_isfunction( L, -1 );// or lua_iscfunction( L, -1 );
// 					if ( /*not lua_isfunction( L, -1 ) and not lua_iscfunction( L, -1 ) and*/ key != "class" and key != "super" ) {
					if ( /*not lua_isfunction( L, -1 ) and not lua_iscfunction( L, -1 ) and*/ key != "super" and key != "self" and key.substr(0, 2) != "__" ) {
						table[key] = value( L, -1 );
					} else {
// 						dont_pop = false;
					}
// 					if ( not dont_pop ) {
						lua_pop( L, 1 );
// 					}
				}
			}
			if ( istop ) {
				lua_pop( L, 1 );
			}
		}
		return table;
	}

	return LuaValue();
}


LuaValue Lua::value( const string& name )
{
	if ( LocateValue( name ) < 0 ) {
		// std::cout << "Lua variable \"" << name << "\" not found\n";
	}
	return value( mLuaState, -1 );
}


LuaValue Lua::value( int index, lua_State* L )
{
	if ( not L ) {
		L = mLuaState;
	}
	return value( L, index );
}


int32_t _lua_bridge( lua_State* L )
{
	int32_t argc = lua_gettop(L);
	vector<LuaValue> args;
	args.resize( argc );
	for ( int32_t i = 0; i < argc; i++ ) {
// 		args[i] = mStates[L]->value( i + 1 );
		lua_pushvalue( L, i + 1 );
		bool dont_pop = lua_isfunction( L, -1 );
		args[i] = Lua::states()[L]->value( -1 );
		if ( not dont_pop ) {
			lua_pop( L, 1 );
		}
	}

	const function<LuaValue(const vector<LuaValue>&)>& fnptr = *static_cast <const function<LuaValue(const vector<LuaValue>&)>*>( lua_touserdata( L, lua_upvalueindex(1) ) );
	LuaValue ret = fnptr( args );

	ret.push( L );
	return 1;
}


int32_t _lua_bridge_raw( lua_State* L )
{
	const function<int(lua_State*)>& fnptr = *static_cast <const function<int(lua_State*)>*>( lua_touserdata( L, lua_upvalueindex(1) ) );
	return fnptr( L );
}


int32_t Lua::RegisterFunction( const string& name, const function<LuaValue(const vector<LuaValue>&)>& func )
{
	new (lua_newuserdata( mLuaState, sizeof(function<LuaValue(const vector<LuaValue>&)>) ) ) function<LuaValue(const vector<LuaValue>&)> (func);
	lua_pushcclosure( mLuaState, &_lua_bridge, 1 );
	lua_setglobal( mLuaState, name.c_str() );

	return 0;
}


int32_t Lua::RegisterMember( const string& object, const string& name, const function<LuaValue(const vector<LuaValue>&)>& func )
{
	if ( LocateValue( object ) < 0 ) {
		return -1;
	}

	new (lua_newuserdata( mLuaState, sizeof(function<LuaValue(const vector<LuaValue>&)>) ) ) function<LuaValue(const vector<LuaValue>&)> (func);
	lua_pushcclosure( mLuaState, &_lua_bridge, 1 );
	lua_setfield( mLuaState, -2, name.c_str() );

	return 0;
}


LuaValue Lua::CallFunction( const string& func, const vector<LuaValue>& args )
{
	mMutex.lock();
	int nArgs = args.size();
	char* funcname = strdup( func.c_str() );

	lua_getglobal( mLuaState, "call_error" );
	int lua_top_base = lua_gettop( mLuaState );
/*
	if ( !strchr(funcname, '.') && !strchr(funcname, ':') ) {
		lua_getglobal( mLuaState, funcname );
	} else {
		char tmp[128];
		int i, j, k;
		bool method = false;
		for ( i=0, j=0, k=0; funcname[i]; i++ ) {
			if ( funcname[i] == '.' || funcname[i] == ':' || funcname[i] == '[' || funcname[i] == ']' ) {
				if ( j == 0 ) {
					continue;
				}
				tmp[j] = 0;
				if ( k == 0 ) {
					lua_getglobal( mLuaState, tmp );
				}else{
					lua_getfield( mLuaState, -1, tmp );
				//	lua_remove( mLuaState, -2);
				}
				memset( tmp, 0, sizeof(tmp));
				j = 0;
				k++;
				method = (funcname[i] == ':');
			} else {
				tmp[j] = funcname[i];
				j++;
			}
		}
		tmp[j] = 0;
		if ( j > 0 ) {
			lua_getfield( mLuaState, -1, tmp );
		} else {
			lua_remove( mLuaState, -2 );
		}
		if ( method ) {
			nArgs++;
		}
	}
*/
	int found = LocateValue( func );
	if ( found < 0 ) {
		std::cout << "WARNING : " << "Lua function not found : " << func;
		mMutex.unlock();
		return LuaValue();
	}

	for( const LuaValue& v : args ) {
		v.push( mLuaState );
	}

	LuaValue value_ret;

	if ( strstr( funcname, ":" ) ) {
		strstr( funcname, ":" )[0] = 0;
	}
	mLastCallName = funcname;
	free( funcname );

	int ret = lua_pcall( mLuaState, nArgs, LUA_MULTRET, -2 - nArgs );
	if ( ret != 0 ) {
		std::cout << "ERROR : " << lua_tostring( mLuaState, -1 );
		lua_pop( mLuaState, -1 );
	} else {
		int32_t nRets = ( lua_gettop( mLuaState ) - 1 ) - lua_top_base;
		if ( nRets == 1 ) {
			value_ret = value( -1 );
		} else if ( nRets > 1 ) {
			for ( int32_t i = 0; i < nRets; i++ ) {
				value_ret[nRets - i - 1] = value( -1 - i );
			}
		}
		lua_pop( mLuaState, nRets );
	}

// 	lua_pop( mLuaState, -1 );
	mMutex.unlock();
	return value_ret;
}

mutex mut;

LuaValue Lua::CallFunction( int32_t registry_index, const vector<LuaValue>& args )
{
	mMutex.lock();
	mLastError = "";
	lua_getglobal( mLuaState, "call_error" );
	int lua_top_base = lua_gettop( mLuaState );
	int nArgs = args.size();

	lua_rawgeti( mLuaState, LUA_REGISTRYINDEX, registry_index );

	for( const LuaValue& v : args ) {
		v.push( mLuaState );
	}

	LuaValue value_ret;
/*
	lua_Debug dbg;
	lua_getstack( mLuaState, 0, &dbg );
	if ( strstr( funcname, ":" ) ) {
		strstr( funcname, ":" )[0] = 0;
	}
	dbg.name = funcname;
	free( funcname );
*/
	mLastCallName = "";

	int ret = lua_pcall( mLuaState, nArgs, LUA_MULTRET, -2 - nArgs );
	if ( ret != 0 ) {
// 		std::cout << "ERROR : " << "(" << ret << ") " << lua_tostring( mLuaState, -1 );
		lua_pop( mLuaState, -1 );
	} else {
		int32_t nRets = lua_gettop( mLuaState ) - lua_top_base;
		if ( nRets == 1 ) {
			value_ret = value( -1, mLuaState );
		} else if ( nRets > 1 ) {
			for ( int32_t i = 0; i < nRets; i++ ) {
				value_ret[nRets - i - 1] = value( -1 - i, mLuaState );
				printf( "value_ret[%d] = \"%s\"\n", nRets-i-1, value_ret[nRets - i - 1].toString().c_str() );
			}
		}
		lua_pop( mLuaState, nRets );
	}

// 	lua_pop( mLuaState, -1 );
	mMutex.unlock();
	return value_ret;
}


int Lua::LocateValue( const string& _name )
{
	const char* name = _name.c_str();

	int top = lua_gettop( mLuaState );

	if ( strchr( name, '.' ) == nullptr and strchr( name, '[' ) == nullptr ) {
		lua_getglobal( mLuaState, name );
		if ( lua_type( mLuaState, -1 ) == LUA_TNIL ) {
			return -1;
		}
	} else {
		char tmp[128];
		int i=0, j, k;
		bool in_table = false;
		for ( i = 0, j = 0, k = 0; name[i]; i++ ) {
			if ( name[i] == '.' or name[i] == '[' or name[i] == ']' ) {
				tmp[j] = 0;
				if ( strlen(tmp) == 0 ) {
					j = 0;
					k++;
					continue;
				}
				if ( name[i] == '[' ) {
					in_table = true;
				}
				if ( in_table and name[i] == ']' and lua_istable( mLuaState, -1 ) ) {
					if ( tmp[0] >= '0' and tmp[0] <= '9' ) {
						lua_rawgeti( mLuaState, -1, atoi(tmp) );
					} else {
						lua_getfield( mLuaState, -1, tmp );
					}
				} else {
					if ( k == 0 ) {
						lua_getglobal( mLuaState, tmp );
					} else {
						lua_getfield( mLuaState, -1, tmp );
					}
				}
				if ( lua_type( mLuaState, -1 ) == LUA_TNIL ) {
					return -1;
				}
				memset( tmp, 0, sizeof( tmp ) );
				j = 0;
				k++;
			} else {
				tmp[j] = name[i];
				j++;
			}
		}
		tmp[j] = 0;
		if ( tmp[0] != 0 ) {
			lua_getfield( mLuaState, -1, tmp );
			if ( lua_type( mLuaState, -1 ) == LUA_TNIL ) {
				return -1;
			}
		}
	}

	lua_pushvalue( mLuaState, -1 );
	lua_replace( mLuaState, top + 1 );
	lua_settop( mLuaState, top + 1 );
	return 0;
}
