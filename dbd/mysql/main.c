#include "dbd_mysql.h"

int dbd_mysql_connection(lua_State *L);
int dbd_mysql_statement(lua_State *L);

/*
 * library entry point
 */
LUA_EXPORT int luaopen_dbd_mysql(lua_State *L) {
    dbd_mysql_statement(L);
    dbd_mysql_connection(L);

    return 1;
}

