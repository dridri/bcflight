#ifndef LUA_H
#define LUA_H

#include <unistd.h>
#include <string.h>
#include <vector>
#include <map>
#include <list>
#include <string>
#include <type_traits>
#include <functional>
#include <utility>
#include <cmath>
#include <atomic>
#include <set>
#include <iostream>
#include <mutex>
extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
};
#include "traits.hpp"


#ifdef LUA_JITLIBNAME
#define lua_isinteger(l, x) ( lua_isnumber(l, x) and fmod((float)lua_tonumber(l, x), 1.0f) == 0.0f )
#else
#define lua_objlen lua_rawlen
#endif


using namespace std;
using namespace utils;

class JSonValue;
class Lua;

int32_t _lua_bridge( lua_State* L );
int32_t _lua_bridge_raw( lua_State* L );

#ifdef __APPLE__
#undef Nil
#endif

class LuaValue
{
public:
	typedef enum {
		Nil,
		Boolean,
		Integer,
		Number,
		String,
		Table,
		Function,
		CFunction,
		UserData,
		Reference
	} Types;
	~LuaValue() {
		if ( mFunctionRef ) {
			mFunctionRef->Unref();
			mFunctionRef = nullptr;
		}
	}
	LuaValue( const LuaValue& v ) : mType(v.mType), mBoolean(v.mBoolean), mInteger(v.mInteger), mNumber(v.mNumber), mString(v.mString), mMap(v.mMap), mCFunctionRaw(v.mCFunctionRaw), mCFunction(v.mCFunction), mUserData(v.mUserData), mReference(v.mReference) {
		mFunctionRef = v.mFunctionRef;
		if ( mFunctionRef ) {
			mFunctionRef->More();
		}
	}
	LuaValue() : mType(Nil), mBoolean(false), mInteger(0), mNumber(0), mString(""), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( bool v ) : mType(Boolean), mBoolean(v), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( double v ) : mType(Number), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( float v ) : mType(Number), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
#if (!defined(ANDROID) && !defined(BOARD_rpi) && !(TARGET_CPU_BITS == 32))
	LuaValue( int32_t v ) : mType(Integer), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
#endif
	LuaValue( uint32_t v ) : mType(Integer), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( uint64_t v ) : mType(Integer), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( lua_Integer v ) : mType(Integer), mBoolean(v!=0), mInteger(v), mNumber(v), mString(to_string(v)), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( const string& v ) : mType(String), mBoolean(v.length()>0), mInteger(0), mNumber(0), mString(v), mFunctionRef(nullptr), mUserData(nullptr) {}
	LuaValue( const char* v, size_t len = 0 ) : mType(String), mBoolean(len>0 or (v and strlen(v)>0)), mInteger(0), mNumber(0), mString(len == 0 and v ? v : ""), mFunctionRef(nullptr), mUserData(nullptr) {
		if ( len > 0 ) {
			for ( size_t i = 0; i < len; i++ ) {
				mString.push_back( v[i] );
			}
		}
	}
	template< typename T1, typename T2> LuaValue( const map< T1, T2 >& m ) : mType(Table), mBoolean(m.size()>0), mInteger(0), mNumber(0), mString(""), mFunctionRef(nullptr), mUserData(nullptr) {
		for ( pair< T1, T2 > entry : m ) {
			mMap.insert( make_pair( entry.first, entry.second ) );
		}
	}
	template< typename T> LuaValue( const vector<T>& v ) : mType(Table), mBoolean(v.size()>0), mInteger(0), mNumber(0), mString(""), mFunctionRef(nullptr), mUserData(nullptr) {
		for ( uint32_t i = 0; i < v.size(); i++ ) {
			mMap.insert( make_pair( to_string( i + 1 ), LuaValue( v[i] ) ) );
		}
	}
	template< typename T> LuaValue( const list<T>& v ) : mType(Table), mBoolean(v.size()>0), mInteger(0), mNumber(0), mString(""), mFunctionRef(nullptr), mUserData(nullptr) {
		uint32_t i = 0;
		for ( T value : v ) {
			mMap.insert( make_pair( to_string( ++i ), value ) );
		}
	}
	template<typename FuncType> LuaValue( FuncType func ) : mType(CFunction), mBoolean(true), mInteger(0), mNumber(0), mString(""), mFunctionRef(nullptr), mUserData(nullptr)
	{
		mCFunction = (const function<LuaValue(const vector<LuaValue>&)>&) [this,func]( const vector<LuaValue>& args ) {
			typedef ftraits<decltype(func)> traits;
			static const uint32_t vargs = traits::arity;
			LuaValue* arr = new LuaValue[args.size()];
			for ( uint32_t i = 0; i < std::min( vargs, (uint32_t)args.size() ); i++ ) {
				arr[i] = args[i];
			}
			LuaValue ret = RegisterMember_helper( func, arr, std::make_index_sequence<vargs>{} );
			delete[] arr;
			return ret;
		};
	}

	template< typename T> LuaValue( T* ptr ) : mType(UserData), mBoolean(ptr!=0), mInteger((uintptr_t)ptr), mNumber((uintptr_t)ptr), mString(to_string((uintptr_t)ptr)), mFunctionRef(nullptr), mUserData(ptr) {
		if ( ptr ) {
			LuaValue* val = dynamic_cast< LuaValue* >( ptr );
			if ( val ) {
				*this = *val;
			}
		}
	}
	LuaValue( void* ptr ) : mType(UserData), mBoolean(ptr!=0), mInteger((uintptr_t)ptr), mNumber((uintptr_t)ptr), mString(to_string((uintptr_t)ptr)), mFunctionRef(nullptr), mUserData(ptr) {}
	LuaValue( nullptr_t ptr ) : mType(UserData), mBoolean(ptr!=0), mInteger((uintptr_t)ptr), mNumber((uintptr_t)ptr), mString(to_string((uintptr_t)ptr)), mFunctionRef(nullptr), mUserData(ptr) {}

	static LuaValue fromReference( Lua* lua, lua_Integer ref );
	static LuaValue fromReference( Lua* lua, LuaValue& val );
	static LuaValue fromReference( Lua* lua, LuaValue* val );

	static LuaValue boolean( bool b ) {
		LuaValue v;
		v.mType = Boolean;
		v.mBoolean = b;
		return v;
	}
	static LuaValue ClosureRaw( const function< int( lua_State* ) >& func ) {
		LuaValue ret;
		ret.mType = CFunction;
		ret.mCFunctionRaw = func;
		return ret;
	}
	template<typename FuncType> static LuaValue Closure( FuncType func ) {
		return LuaValue( func );
	}
	template<typename Obj, typename FuncType> static LuaValue Closure( Obj obj, FuncType func ) {
		LuaValue ret;
		ret.mCFunction = (const function<LuaValue(const vector<LuaValue>&)>&) [obj,func]( const vector<LuaValue>& args ) {
			typedef ftraits<decltype(func)> traits;
			static const uint32_t vargs = traits::arity;
			LuaValue* arr = new LuaValue[args.size()];
			for ( uint32_t i = 0; i < std::min( vargs, (uint32_t)args.size() ); i++ ) {
				arr[i] = args[i];
			}
			LuaValue ret = RegisterMember_helper( obj, func, arr, std::make_index_sequence<vargs>{} );
			delete[] arr;
			return ret;
		};
		return ret;
	}
	template< typename T> static LuaValue Create( initializer_list<T> v ) {
		LuaValue ret;
		for ( T value : v ) {
			ret.mMap[value.first] = value.second;
		}
		return ret;
	}
	static LuaValue Value( Lua* state, int32_t index );
	static LuaValue Registry( Lua* state, int32_t index );

	void setFunctionRef( Lua* lua, int32_t r ) {
		mType = Function;
		if ( not mFunctionRef ) {
			mFunctionRef = new FunctionRef();
		}
		mFunctionRef->Ref( lua, r );
	}
	int32_t getFunctionRef() {
		if ( mType != Function or not mFunctionRef ) {
			return -1;
		}
		return mFunctionRef->ref();
	}
	Types type() const { return mType; }
	LuaValue& operator=( const LuaValue& v ) {
		mType = v.mType;
		mBoolean = v.mBoolean;
		mInteger = v.mInteger;
		mNumber = v.mNumber;
		mString = v.mString;
		mMap = v.mMap;
		mCFunctionRaw = v.mCFunctionRaw;
		mCFunction = v.mCFunction;
		mUserData = v.mUserData;
		if ( mFunctionRef ) {
			mFunctionRef->Unref();
		}
		mFunctionRef = v.mFunctionRef;
		if ( mFunctionRef ) {
			mFunctionRef->More();
		}
		mReference = v.mReference;
		return *this;
	}

	void push( lua_State* L ) const {
		switch( mType ) {
			case Boolean : lua_pushboolean( L, mBoolean ); break;
			case Integer : lua_pushinteger( L, mInteger ); break;
			case Number : lua_pushnumber( L, mNumber ); break;
			case String : lua_pushlstring( L, mString.data(), mString.length() ); break;
			case UserData: lua_pushlightuserdata( L, mUserData ); break;
			case Table:
			case Function:
			case CFunction:
			case Reference:
			default: lua_pushnil( L ); break;
		}
		if ( mType == Reference ) {
			if ( mReference.count( L ) > 0 ){
				lua_rawgeti( L, LUA_REGISTRYINDEX, mReference.at(L) );
			}
		} else if ( mType == Table ) {
			lua_createtable( L, 0, 0 );
			for ( pair< string, LuaValue > entry : mMap ) {
				string key = entry.first;
				bool integral = ( ( key[0] == '0' and key.length() == 1 ) or ( key[0] >= '0' and key[0] <= '9' and atoi( key.c_str() ) != 0 ) );
				if ( integral ) {
					lua_pushinteger( L, atoi( entry.first.c_str() ) );
				} else {
					lua_pushstring( L, entry.first.c_str() );
				}
				entry.second.push( L );
				lua_settable( L, -3 );
			}
			// Set table as its own metatable
			lua_pushvalue( L, -1 );
			lua_setmetatable( L, -2 );
		} else if ( mType == Function ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, mFunctionRef->ref() );
		} else if ( mType == CFunction ) {
			if ( mCFunctionRaw ) {
				new (lua_newuserdata( L, sizeof(function<int(lua_State*)>) ) ) function<int(lua_State*)> (mCFunctionRaw);
				lua_pushcclosure( L, &_lua_bridge_raw, 1 );
			} else {
				new (lua_newuserdata( L, sizeof(function<LuaValue(const vector<LuaValue>&)>) ) ) function<LuaValue(const vector<LuaValue>&)> (mCFunction);
				lua_pushcclosure( L, &_lua_bridge, 1 );
			}
		} else if ( mType == UserData ) {
			lua_pushlightuserdata( L, mUserData );
		}
	}

	const LuaValue& operator[]( size_t idx ) const {
		return operator[]( to_string(idx) );
	}
	const LuaValue& operator[]( const string& s ) const {
		if ( mType == Table and mMap.find( s ) != mMap.end() ) {
			return mMap.at( s );
		}
		return mNil;
	}
	const LuaValue& operator[]( const char* s ) const {
		return operator[]( (string)s );
	}
	LuaValue& operator[]( size_t idx ) {
		return operator[]( to_string(idx) );
	}
	LuaValue& operator[]( const string& s ) {
		mType = Table;
		if ( mMap.find( s ) == mMap.end() ) {
			mMap.insert( make_pair( s, LuaValue() ) );
		}
		return mMap[ s ];
	}
	LuaValue& operator[]( const char* s ) {
		if ( s == 0 ) {
			return mNil;
		}
		return operator[]( (string)s );
	}
	string toString() const {
		switch( mType ) {
			case Boolean : return mBoolean ? "true" : "false";
			case Integer : return to_string( mInteger );
			case Number : return to_string( mNumber );
			case String : return mString;
			case Function : return "[lua_function]";
			default: return "";
		}
	}
	lua_Integer toInteger() const {
		switch( mType ) {
			case Boolean : return mBoolean;
			case Integer : return mInteger;
			case Number : return mNumber;
			case String : return atoi( mString.c_str() );
			case UserData : return (uintptr_t)mUserData;
			default: return 0;
		}
	}
	lua_Number toNumber() const {
		switch( mType ) {
			case Boolean : return mBoolean;
			case Integer : return mInteger;
			case Number : return mNumber;
			case String : return atof( mString.c_str() );
			default: return 0;
		}
	}
	bool toBoolean() const {
		switch( mType ) {
			case Boolean : return mBoolean;
			case Integer : return mInteger;
			case Number : return mNumber;
			case String : return ( mString.length() > 0 );
			default: return false;
		}
	}
	const map< string, LuaValue >& toTable() const {
		if ( mType != Table ) {
			return mNil.mMap;
		}
		return mMap;
	}
	void* toUserData() const {
		if ( mType == UserData ) {
			return mUserData;
		}
		return nullptr;
	}

	operator double () {
		return toNumber();
	}
	operator int64_t () const {
		return toInteger();
	}
	explicit operator std::string () const {
		return toString();
	}
	operator void* () const {
		return mUserData;
	}
	template<typename T> operator T* () const {
		return static_cast<T*>( mUserData );
	}
	template< typename ...LuaValues > inline LuaValue operator() ( LuaValues... values ) const {
		if ( mType != Function or mFunctionRef == nullptr ) {
			std::cout << "WARNING : " << "Calling a LuaValue that is not a function !";
			return LuaValue();
		}
		return mFunctionRef->Call( { values... } );
	}
	inline LuaValue operator() ( const vector<LuaValue>& args ) const {
		if ( mType != Function or mFunctionRef == nullptr ) {
			std::cout << "WARNING : " << "Calling a LuaValue that is not a function !";
			return LuaValue();
		}
		return mFunctionRef->Call( args );
	}

	bool operator==( const std::string& s ) const {
		return ( toString() == s );
	}
	bool operator==( const char* s ) const {
		return ( toString() == string(s) );
	}

	bool operator==( const LuaValue& v ) const {
		return ( toString() == v.toString() );
	}

	operator map<string, string> () const {
		map< string, string > ret;
		if ( mType == Table ) {
			for ( auto entry : mMap ) {
				ret[entry.first] = entry.second.toString();
			}
		}
		return ret;
	}
	template<typename T> operator map<string, T> () const {
		map< string, T > ret;
		if ( mType == Table ) {
			for ( auto entry : mMap ) {
				ret[entry.first] = static_cast<T>( entry.second );
			}
		}
		return ret;
	}

	template<typename T> operator vector<T> () const {
		vector< T > ret;
		if ( mType == Table ) {
			for ( auto entry : mMap ) {
				ret.emplace_back( static_cast<T>( entry.second ) );
			}
		}
		return ret;
	}

	template<typename T> operator list<T> () const {
		list< T > ret;
		if ( mType == Table ) {
			for ( auto entry : mMap ) {
				ret.emplace_back( static_cast<T>( entry.second ) );
			}
		}
		return ret;
	}

	string serialize( uint32_t indent = 0 ) const {
		string ret = "";
		switch( mType ) {
			case Boolean : ret += ( mBoolean ? "true" : "false" ); break;
			case Integer : ret += to_string( mInteger ); break;
			case Number : ret += to_string( mNumber ); break;
			case String : ret += "\"" + mString + "\""; break;
			case Nil : ret += "nil"; break;
			default: break;
		}
		if ( mType == Table ) {
			ret = "{\n";
			for ( pair< string, LuaValue > entry : mMap ) {
				for ( uint32_t i = 0; i < indent + 1; i++ ) ret += "    ";
				string key = entry.first;
				bool integral = ( ( key[0] == '0' and key.length() == 1 ) or ( key[0] >= '0' and key[0] <= '9' and atoi( key.c_str() ) != 0 ) );
				if ( integral ) {
					ret += "[" + key + "]";
				} else {
					ret += key;
				}
				ret += " = " + entry.second.serialize( indent + 1 ) + ",\n";
			}
			for ( uint32_t i = 0; i < indent; i++ ) ret += "    ";
			ret += "}";
		}
		return ret;
	}

protected:
	class FunctionRef {
	public:
		FunctionRef() : mCounter(0), mRef(-1) {}
		FunctionRef( const FunctionRef& r ) : mCounter(r.mCounter.operator unsigned int()), mRef(r.mRef) {}
		~FunctionRef();
		FunctionRef& operator=( const FunctionRef& r ) { mCounter = r.mCounter.operator unsigned int(); mRef = r.mRef; return *this; }
		int32_t ref() { return mRef; }
		void Ref( Lua* lua, int32_t r ) {
			mLua = lua;
			mRef = r;
			mCounter++;
		}
		void Unref();
		void More() {
			mCounter++;
		}
		LuaValue Call( const vector<LuaValue>& args );
	private:
		atomic<uint32_t> mCounter;
		Lua* mLua;
		int32_t mRef;
	};
	Types mType;
	bool mBoolean;
	int64_t mInteger;
	double mNumber;
	string mString;
	map< string, LuaValue > mMap;
	function< int( lua_State* ) > mCFunctionRaw;
	function<LuaValue(const vector<LuaValue>&)> mCFunction;
	FunctionRef* mFunctionRef;
	void* mUserData;
	map< lua_State*, int32_t > mReference;
	static LuaValue mNil;

protected:
	template <typename T>
	struct ftraits
		: public function_traits<T>
	{};

	template <typename ClassType, typename ReturnType, typename... Args>
	struct ftraits<ReturnType(ClassType::*)(Args...) const>
	{
		enum { arity = sizeof...(Args) };
		typedef ReturnType result_type;
		template <size_t i>
		struct arg {
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

	template <std::size_t... I, typename Obj, typename FuncType> static LuaValue RegisterMember_helper( Obj obj, FuncType func, const LuaValue* data, std::index_sequence<I...> ) {
// 		auto f = std::bind( func, obj, std::_Placeholder<I+1>{}... );
// 		return f(data[I]...);
		return func(obj, data[I]...);
	}

	template <std::size_t... I, typename FuncType> static LuaValue RegisterMember_helper( FuncType func, const LuaValue* data, std::index_sequence<I...> ) {
		return func(data[I]...);
	}
};

class Lua
{
public:
	Lua( const string& load_path = "" );
	~Lua();

	void Reload();
	lua_State* state() const { return mLuaState; }
	vector<lua_Debug> trace();
	void setExitOnError( bool en ) { mExitOnError = en; }
	bool exitOnError() { return mExitOnError; }
	void addPathTip( const string& path ) { mPathTips.push_back( path ); }

	int32_t do_file( const string& filename, const string& path_tip = "" );
	int32_t do_string( const string& str );

	int32_t RegisterFunction( const string& name, const function<LuaValue(const vector<LuaValue>&)>& func );
	int32_t RegisterMember( const string& object, const string& name, const function<LuaValue(const vector<LuaValue>&)>& func );

	template<typename Obj, typename FuncType> int32_t RegisterMember( const string& object, const string& name, Obj obj, FuncType func ) {
		return RegisterMember( object, name, (const function<LuaValue(const vector<LuaValue>&)>&) [this,obj,func]( const vector<LuaValue>& args ) {
			typedef ftraits<FuncType> traits;
			static const uint32_t vargs = traits::arity;
			LuaValue* arr = new LuaValue[args.size()];
			for ( uint32_t i = 0; i < std::min( vargs, (uint32_t)args.size() ); i++ ) {
				arr[i] = args[i];
			}
			LuaValue ret = RegisterMember_helper( obj, func, arr, std::make_index_sequence<vargs>{} );
			delete[] arr;
			return ret;
		});
	}

	template<typename FuncType> int32_t RegisterMember( const string& object, const string& name, FuncType func ) {
		return RegisterMember( object, name, (const function<LuaValue(const vector<LuaValue>&)>&) [this,func]( const vector<LuaValue>& args ) {
			typedef ftraits<decltype(func)> traits;
			static const uint32_t vargs = traits::arity;
			LuaValue* arr = new LuaValue[args.size()];
			for ( uint32_t i = 0; i < std::min( vargs, (uint32_t)args.size() ); i++ ) {
				arr[i] = args[i];
			}
			LuaValue ret = RegisterMember_helper( func, arr, std::make_index_sequence<vargs>{} );
			delete[] arr;
			return ret;
		});
	}

	template<typename Obj, typename FuncType> int32_t RegisterFunction( const string& name, Obj obj, FuncType func ) {
		return RegisterMember( "_G", name, obj, func );
	}

	template<typename FuncType> int32_t RegisterFunction( const string& name, FuncType func ) {
		return RegisterMember( "_G", name, func );
	}

	template<typename ...Types> function<LuaValue(Types...)> ExtractMember( const string& object, const string& member = "" ) {
		return [this,object,member] ( Types... args ) {
			return CallFunction( object + ( member != "" ? ":" : "" ) + member, args... );
		};
	}

// 	int32_t CallFunction( const string& funcname );
// 	int32_t CallFunction( const string& funcname, const string& fmt, ... );
	LuaValue CallFunction( const string& funcname, const vector<LuaValue>& args );
	LuaValue CallFunction( int32_t registry_index, const vector<LuaValue>& args );
	template< typename ...LuaValues > LuaValue CallFunction( const string& funcname, LuaValues... values ) {
		return CallFunction( funcname, { values... } );
	}
	void push( const LuaValue& v ) {
		v.push( mLuaState );
	}
	LuaValue value( const string& name );
	LuaValue value( int index = -1, lua_State* L = nullptr );
	static LuaValue value( lua_State* L, int index = -1 );
	template<typename T> static T value( lua_State* L, int index, int top, const T& def ) {
		if ( index < top and lua_type( L, index ) != LUA_TNONE ) {
			return (T)value( L, index );
		}
		return def;
	}

	std::mutex& mutex() { return mMutex; }
	const string& lastCallName() const { return mLastCallName; }
	const string& lastError() const { return mLastError; }
	const set< string >& openedFiles() const { return mOpenedFiles; }
	static map< lua_State*, Lua* >& states() { return mStates; }

protected:
	template <typename T>
	struct ftraits
		: public function_traits<T>
	{};

	template <typename ClassType, typename ReturnType, typename... Args>
	struct ftraits<ReturnType(ClassType::*)(Args...) const>
	{
		enum { arity = sizeof...(Args) };
		typedef ReturnType result_type;
		template <size_t i>
		struct arg {
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

	template <std::size_t... I, typename Obj, typename FuncType> LuaValue RegisterMember_helper( Obj obj, FuncType func, const LuaValue* data, std::index_sequence<I...> ) {
// 		auto f = std::bind( func, obj, std::_Placeholder<I+1>{}... );
// 		return f(data[I]...);
		return func(obj, data[I]...);
	}

	template <std::size_t... I, typename FuncType> LuaValue RegisterMember_helper( FuncType func, const LuaValue* data, std::index_sequence<I...> ) {
		return func(data[I]...);
	}

	int LocateValue( const string& _name );
	static int CallError( lua_State* L );
	static int Traceback( lua_State* L );

	lua_State* mLuaState;
	std::mutex mMutex;
	set< string > mOpenedFiles;
	list< string > mPathTips;
	string mLastCallName;
	string mLastError;
	bool mExitOnError;
	static map< lua_State*, Lua* > mStates;

};


#define LUA_CLASS
#define LUA_EXPORT
#define LUA_MEMBER
#define LUA_PROPERTY(...)

#endif // LUA_H
