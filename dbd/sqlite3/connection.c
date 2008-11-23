#include "dbd_sqlite3.h"

int dbd_sqlite3_statement_create(lua_State *L, connection_t *conn, const char *sql_query);

static int connection_new(lua_State *L) {
    int n = lua_gettop(L);

    const char *db = NULL;
    connection_t *conn = NULL;

    /* db */
    switch (n) {
    default:
	if (lua_isnil(L, 1) == 0) 
	    db = luaL_checkstring(L, 1);
    }

    conn = (connection_t *)lua_newuserdata(L, sizeof(connection_t));

    if (sqlite3_open(db, &conn->sqlite) == SQLITE_OK) {
        luaL_getmetatable(L, DBD_SQLITE_CONNECTION);
        lua_setmetatable(L, -2);
    } else {
	luaL_error(L, "Failed to connect to database: %s", sqlite3_errmsg(conn->sqlite));
	lua_pushnil(L);
    }

    return 1;
}

static int connection_close(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_SQLITE_CONNECTION);
    int disconnect = 0;   

    if (conn->sqlite) {
#if 0
	sqlite3_stmt *stmt;

	while((stmt = sqlite3_next_stmt(conn->sqlite, NULL)) != 0){
	    sqlite3_finalize(stmt);
	}
#endif
	if (sqlite3_close(conn->sqlite) == SQLITE_OK) {
	    disconnect = 1;
	}
    }

    lua_pushboolean(L, disconnect);
    return 1;
}

static int connection_ping(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_SQLITE_CONNECTION);
    int ok = 0;   

    if (conn->sqlite) {
	ok = 1;
    }

    lua_pushboolean(L, ok);
    return 1;
}

static int connection_prepare(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_SQLITE_CONNECTION);

    if (conn->sqlite) {
	return dbd_sqlite3_statement_create(L, conn, luaL_checkstring(L, 2));
    }

    lua_pushnil(L);    
    return 1;
}


static int connection_gc(lua_State *L) {
    /* always close the connection */
    connection_close(L);

    return 0;
}

static const luaL_Reg connection_methods[] = {
    {"close", connection_close},
    {"ping", connection_ping},
    {"prepare", connection_prepare},
    {NULL, NULL}
};

static const luaL_Reg connection_class_methods[] = {
    {"New", connection_new},
    {NULL, NULL}
};

int dbd_sqlite3_connection(lua_State *L) {
    luaL_newmetatable(L, DBD_SQLITE_CONNECTION);
    luaL_register(L, 0, connection_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, connection_gc);
    lua_setfield(L, -2, "__gc");

    luaL_register(L, DBD_SQLITE_CONNECTION, connection_class_methods);

    return 1;    
}
