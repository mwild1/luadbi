#include "dbd_postgresql.h"

int dbd_postgresql_connection(lua_State *L);
int dbd_postgresql_statement(lua_State *L);

/*
 * library entry point
 */
LUA_EXPORT int luaopen_dbd_postgresql(lua_State *L) {
	dbd_postgresql_statement(L);
	dbd_postgresql_connection(L);

	return 1;
}

