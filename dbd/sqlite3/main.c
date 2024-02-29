#include "dbd_sqlite3.h"

int dbd_sqlite3_connection(lua_State *L);
int dbd_sqlite3_statement(lua_State *L);

/*
 * library entry point
 */
LUA_EXPORT int luaopen_dbd_sqlite3(lua_State *L) {
	dbd_sqlite3_statement(L);
	dbd_sqlite3_connection(L);

	return 1;
}

