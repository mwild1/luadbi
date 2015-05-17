#include "dbd_oracle.h"

int dbd_oracle_connection(lua_State *L);
int dbd_oracle_statement(lua_State *L);

/* 
 * library entry point
 */
LUA_EXPORT int luaopen_dbdoracle(lua_State *L) {
    dbd_oracle_statement(L); 
    dbd_oracle_connection(L);

    return 1;
}

