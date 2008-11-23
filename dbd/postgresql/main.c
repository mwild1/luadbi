#include "dbd_postgresql.h"

int dbd_postgresql_connection(lua_State *L);
int dbd_postgresql_statement(lua_State *L);

int luaopen_dbdpostgresql(lua_State *L) {
    dbd_postgresql_connection(L);
    dbd_postgresql_statement(L); 

    return 1;
}

