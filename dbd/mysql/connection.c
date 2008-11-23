#include "dbd_mysql.h"

int dbd_mysql_statement_create(lua_State *L, connection_t *conn, const char *sql_query);

static int connection_new(lua_State *L) {
    int n = lua_gettop(L);

    connection_t *conn = NULL;

    const char *host = NULL;
    const char *user = NULL;
    const char *password = NULL;
    const char *db = NULL;
    int port = 0;

    const char *unix_socket = NULL; /* TODO always NULL */
    int client_flag = 0; /* TODO always 0, set flags from options table */

    /* db, user, password, host, port */
    switch (n) {
    case 5:
	if (lua_isnil(L, 5) == 0) 
	    port = luaL_checkint(L, 5);
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

    conn->mysql = mysql_init(NULL);

    if (mysql_real_connect(conn->mysql, host, user, password, db, port, unix_socket, client_flag)) {
        luaL_getmetatable(L, DBD_MYSQL_CONNECTION);
        lua_setmetatable(L, -2);
    } else {
	luaL_error(L, "Failed to connect to database: %s", mysql_error(conn->mysql));
	lua_pushnil(L);
    }

    return 1;
}

static int connection_close(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_MYSQL_CONNECTION);
    int disconnect = 0;   

    if (conn->mysql) {
	mysql_close(conn->mysql);
	disconnect = 1;
    }

    lua_pushboolean(L, disconnect);
    return 1;
}

static int connection_ping(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_MYSQL_CONNECTION);
    int err = 1;   

    if (conn->mysql) {
	err = mysql_ping(conn->mysql);
    }

    lua_pushboolean(L, !err);
    return 1;
}

static int connection_prepare(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_MYSQL_CONNECTION);

    if (conn->mysql) {
	return dbd_mysql_statement_create(L, conn, luaL_checkstring(L, 2)); 
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

int dbd_mysql_connection(lua_State *L) {
    luaL_newmetatable(L, DBD_MYSQL_CONNECTION);
    luaL_register(L, 0, connection_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, connection_gc);
    lua_setfield(L, -2, "__gc");

    luaL_register(L, DBD_MYSQL_CONNECTION, connection_class_methods);

    return 1;    
}

