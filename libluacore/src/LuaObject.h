#ifndef LUAOBJECT_H
#define LUAOBJECT_H

#include "Lua.h"

class LuaObject
{
public:
	LuaObject( Lua* state );
	~LuaObject();

protected:
	Lua* mState;
	int64_t mReference;
};

#endif // LUAOBJECT_H
