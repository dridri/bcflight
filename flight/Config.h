#ifndef CONFIG_H
#define CONFIG_H

#include <string>

extern "C" {
#include <luajit/luajit.h>
#include <luajit/lua.h>
#include <luajit/lualib.h>
#include <luajit/lauxlib.h>
};

class Config
{
public:
	Config( const std::string& filename );
	~Config();

	void Reload();
	void Save();

	std::string string( const std::string& name );
	int integer( const std::string& name );

protected:
	std::string mFilename;
	lua_State* L;
	int LocateValue( const std::string& name );
};

#endif // CONFIG_H
