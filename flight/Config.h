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
#include <Lua.h>

using namespace STD;

class Config
{
public:
	Config( const string& filename, const string& settings_filename = "" );
	~Config();

	void Reload();
	void Apply();
	void Save();
	void Execute( const string& code, bool silent = false );

	string String( const string& name, const string& def = "" );
	int Integer( const string& name, int def = 0 );
	float Number( const string& name, float def = 0.0f );
	bool Boolean( const string& name, bool def = false );
	void* Object( const string& name, void* def = nullptr );
	template<typename T> T* Object( const string& name, void* def = nullptr ) {
		return static_cast<T*>( Object( name, def ) );
	}

	vector<int> IntegerArray( const string& name );

	string DumpVariable( const string& name, int index = -1, int indent = 0 );
	int ArrayLength( const string& name );

	void addOverride( const string& key, const string& value );
	void setBoolean( const string& name, const bool v );
	void setInteger( const string& name, const int v );
	void setNumber( const string& name, const float v );
	void setString( const string& name, const string& v );

	string ReadFile();
	void WriteFile( const string& content );

	Lua* luaState() const;

	static Config* instance() { return sConfig; }

protected:
	int LocateValue( const string& name );

	string mFilename;
	string mSettingsFilename;
	// lua_State* L;
	map< string, string > mSettings;
	map< string, string > mOverrides;

	Lua* mLua;
	static Config* sConfig;
};

#endif // CONFIG_H
