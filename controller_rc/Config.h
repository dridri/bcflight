/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>

extern "C" {
#include <luajit.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};

class Config
{
public:
	Config( const std::string& filename );
	~Config();

	void Reload();
	void Save();

	std::string string( const std::string& name, const std::string& def = "" );
	int integer( const std::string& name, int def = 0 );
	float number( const std::string& name, float def = 0.0f );
	bool boolean( const std::string& name, bool def = false );
	std::vector<int> integerArray( const std::string& name );

	void DumpVariable( const std::string& name, int index = -1, int indent = 0 );

	const bool setting( const std::string& name, const bool def ) const;
	const int setting( const std::string& name, const int def ) const;
	const float setting( const std::string& name, const float def ) const;
	const std::string& setting( const std::string& name, const std::string& def = "" ) const;
	void setSetting( const std::string& name, const bool v );
	void setSetting( const std::string& name, const int v );
	void setSetting( const std::string& name, const float v );
	void setSetting( const std::string& name, const std::string& v );
	void LoadSettings( const std::string& filename = "settings" );
	void SaveSettings( const std::string& filename = "settings" );

	std::string ReadFile();
	void WriteFile( const std::string& content );

protected:
	std::string mFilename;
	lua_State* L;
	int LocateValue( const std::string& name );

	std::map< std::string, std::string > mSettings;
};

#endif // CONFIG_H
