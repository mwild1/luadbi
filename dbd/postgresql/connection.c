#include "dbd_postgresql.h"

int dbd_postgresql_statement_create(lua_State *L, connection_t *conn, const char *sql_query);

/*
 * connection = DBD.PostgreSQL.New(dbname, user, password, host, port)
 */
static int connection_new(lua_State *L) {
    int n = lua_gettop(L);
    connection_t *conn = NULL;

    const char *host = NULL;
    const char *user = NULL;
    const char *password = NULL;
    const char *db = NULL;
    const char *port = NULL;

    const char *options = NULL; /* TODO always NULL */
    const char *tty = NULL; /* TODO always NULL */

    char portbuf[18];

    /* db, user, password, host, port */
    switch (n) {
    case 5:
	if (lua_isnil(L, 5) == 0) 
	{
	    int pport = luaL_checkint(L, 5);

	    if (pport >= 1 && pport <= 65535) {
		snprintf(portbuf, sizeof(portbuf), "%d", pport);
		port = portbuf;
	    } else {
		luaL_error(L, "Invalid port %d", pport);
	    }
	}
    case 4: 
	if (lua_isnil(L, 4) == 0) 
	    host = luaL_checkstring(L, 4);
    case 3:
	if (lua_isnil(L, 3) == 0) 
	    password = luaL_checkstring(L, 3);
    case 2:
	if (lua_isnil(L, 2) == 0) 
	    user = luaL_checkstring(L, 2);
    case 1:
	if (lua_isnil(L, 1) == 0) 
	    db = luaL_checkstring(L, 1);
    }

    conn = (connection_t *)lua_newuserdata(L, sizeof(connection_t));

    conn->postgresql = PQsetdbLogin(host, port, options, tty, db, user, password);
    conn->statement_id = 0;

    if (PQstatus(conn->postgresql) == CONNECTION_OK) {
        luaL_getmetatable(L, DBD_POSTGRESQL_CONNECTION);
        lua_setmetatable(L, -2);
    } else {
	luaL_error(L, "Failed to connect to database: %s", PQerrorMessage(conn->postgresql));
	lua_pushnil(L);
    }

    return 1;
}

/*
 * success = connection:close()
 */
static int connection_close(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_CONNECTION);
    int disconnect = 0;   

    if (conn->postgresql) {
	PQfinish(conn->postgresql);
	disconnect = 1;
    }

    lua_pushboolean(L, disconnect);
    return 1;
}

/*
 * ok = connection:ping()
 */
static int connection_ping(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_CONNECTION);
    int ok = 0;   

    if (conn->postgresql) {
	ConnStatusType status = PQstatus(conn->postgresql);

	if (status == CONNECTION_OK)
	    ok = 1;
    }

    lua_pushboolean(L, ok);
    return 1;
}

/*
 * statement = connection:prepare(sql_string)
 */
static int connection_prepare(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_CONNECTION);

    if (conn->postgresql) {
	return dbd_postgresql_statement_create(L, conn, luaL_checkstring(L, 2));
    }

    lua_pushnil(L);    
    return 1;
}

/*
 * __gc
 */
static int connection_gc(lua_State *L) {
    /* always close the connection */
    connection_close(L);

    return 0;
}

int dbd_postgresql_connection(lua_State *L) {
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

    luaL_newmetatable(L, DBD_POSTGRESQL_CONNECTION);
    luaL_register(L, 0, connection_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, connection_gc);
    lua_setfield(L, -2, "__gc");

    luaL_register(L, DBD_POSTGRESQL_CONNECTION, connection_class_methods);

    return 1;    
}
