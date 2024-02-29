#include "dbd_db2.h"

int dbd_db2_connection(lua_State *L);
int dbd_db2_statement(lua_State *L);

/*
 * library entry point
 */
LUA_EXPORT int luaopen_dbd_db2(lua_State *L) {
	dbd_db2_statement(L);
	dbd_db2_connection(L);

	return 1;
}

