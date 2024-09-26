#include "dbd_duckdb.h"

int dbd_duckdb_connection(lua_State *L);
int dbd_duckdb_statement(lua_State *L);

/*
 * library entry point
 */
LUA_EXPORT int luaopen_dbd_duckdb(lua_State *L) {
	dbd_duckdb_statement(L);
	dbd_duckdb_connection(L);

	return 1;
}

