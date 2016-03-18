
#define ltmrlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

static int try2 (lua_State *L)
{
	double op1 = luaL_checknumber(L, 1); 
	lua_pushnumber(L, op1 + 1);
	lua_pushnumber(L, op1 + 2);
	lua_pushnumber(L, op1 + 3);
	printf("%d, %d, %d\n", luaL_checknumber(L, 1),
		luaL_checknumber(L, 1),
		luaL_checknumber(L, 1));
	//lua_getglobal(L, "print");
	
	//lua_pushnumber(L, op1);
	//lua_pcall(L, 1, 0, 0);
	return 1;
}

static const luaL_Reg tmrlib[] = {
	{"try2",   try2},
	{NULL, NULL}
};


/*
** Open math library
*/
LUALIB_API int luaopen_tmr (lua_State *L) {
	luaL_register(L, LUA_TMRLIBNAME, tmrlib);
	return 1;
}


