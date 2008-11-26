#include "dbd_sqlite3.h"

/*
 * Converts SQLite types to Lua types
 */
static lua_push_type_t sqlite_to_lua_push(unsigned int sqlite_type) {
    lua_push_type_t lua_type;

    switch(sqlite_type) {
    case SQLITE_NULL:
        lua_type = LUA_PUSH_NIL;
        break;

    case SQLITE_INTEGER:
        lua_type =  LUA_PUSH_INTEGER;
        break;

    case SQLITE_FLOAT:
        lua_type = LUA_PUSH_NUMBER;
        break;

    default:
        lua_type = LUA_PUSH_STRING;
    }

    return lua_type;
}

/*
 * runs sqlite3_step on a statement handle
 */
static int step(statement_t *statement) {
    int res = sqlite3_step(statement->stmt);

    if (res == SQLITE_DONE) {
	statement->more_data = 0;
	return 1;
    } else if (res == SQLITE_ROW) {
	statement->more_data = 1;
	return 1;
    }

    return 0;
}

/*
 * success = statement:close()
 */
int statement_close(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_SQLITE_STATEMENT);
    int ok = 0;

    if (statement->stmt) {
	if (sqlite3_finalize(statement->stmt) == SQLITE_OK) {
	    ok = 1;
	}

	statement->stmt = NULL;
    }

    lua_pushboolean(L, ok);
    return 1;
}

/*
 * success,err = statement:execute(...)
 */
int statement_execute(lua_State *L) {
    int n = lua_gettop(L);
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_SQLITE_STATEMENT);
    int p;
    int err = 0;


    if (!statement->stmt) {
	lua_pushboolean(L, 0);
	lua_pushstring(L, DBI_ERR_EXECUTE_INVALID);
	return 2;
    }

    /*
     * reset the handle before binding params
     * this wil be a NOP if the handle has not
     * been executed
     */
    if (sqlite3_reset(statement->stmt) != SQLITE_OK) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, DBI_ERR_EXECUTE_FAILED, sqlite3_errmsg(statement->sqlite));
	return 2;
    }

    for (p = 2; p <= n; p++) {
	int i = p - 1;

	if (lua_isnil(L, p)) {
	    if (sqlite3_bind_null(statement->stmt, i) != SQLITE_OK) {
		err = 1;
	    }
	} else if (lua_isnumber(L, p)) {
	    if (sqlite3_bind_double(statement->stmt, i, luaL_checknumber(L, p)) != SQLITE_OK) {
		err = 1;
	    }
	} else if (lua_isstring(L, p)) {
	    if (sqlite3_bind_text(statement->stmt, i, luaL_checkstring(L, p), -1, SQLITE_STATIC) != SQLITE_OK) {
		err = 1;
	    }
	}

	if (err)
	    break;
    }   

    if (err) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, sqlite3_errmsg(statement->sqlite));
	return 2;
    }

    if (!step(statement)) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, DBI_ERR_EXECUTE_FAILED, sqlite3_errmsg(statement->sqlite));
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * must be called after an execute
 */
static int statement_fetch_impl(lua_State *L, int named_columns) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_SQLITE_STATEMENT);
    int num_columns;

    if (!statement->stmt) {
	luaL_error(L, DBI_ERR_FETCH_INVALID);
	return 0;
    }

    if (!statement->more_data) {
	/* 
         * Result set is empty, or not result set returned
         */
  
	lua_pushnil(L);
	return 1;
    }

    num_columns = sqlite3_column_count(statement->stmt);

    if (num_columns) {
	int i;
	int d = 1;

	lua_newtable(L);

	for (i = 0; i < num_columns; i++) {
	    lua_push_type_t lua_push = sqlite_to_lua_push(sqlite3_column_type(statement->stmt, i));
	    const char *name = sqlite3_column_name(statement->stmt, i);

	    if (lua_push == LUA_PUSH_NIL) {
                if (named_columns) {
                    LUA_PUSH_ATTRIB_NIL(name);
                } else {
                    LUA_PUSH_ARRAY_NIL(d);
                }
            } else if (lua_push == LUA_PUSH_INTEGER) {
		int val = sqlite3_column_int(statement->stmt, i);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_INT(name, val);
                } else {
                    LUA_PUSH_ARRAY_INT(d, val);
                }
            } else if (lua_push == LUA_PUSH_NUMBER) {
		double val = sqlite3_column_double(statement->stmt, i);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_FLOAT(name, val);
                } else {
                    LUA_PUSH_ARRAY_FLOAT(d, val);
                }
            } else if (lua_push == LUA_PUSH_STRING) {
		const char *val = (const char *)sqlite3_column_text(statement->stmt, i);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_STRING(name, val);
                } else {
                    LUA_PUSH_ARRAY_STRING(d, val);
                }
            } else if (lua_push == LUA_PUSH_BOOLEAN) {
		int val = sqlite3_column_int(statement->stmt, i);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_BOOL(name, val);
                } else {
                    LUA_PUSH_ARRAY_BOOL(d, val);
                }
            } else {
                luaL_error(L, DBI_ERR_UNKNOWN_PUSH);
            }
	}
    } else {
	/* 
         * no columns returned by statement?
         */ 
	lua_pushnil(L);
    }

    if (step(statement) == 0) {
	if (sqlite3_reset(statement->stmt) != SQLITE_OK) {
	    /* 
	     * reset needs to be called to retrieve the 'real' error message
	     */
	    luaL_error(L, DBI_ERR_FETCH_FAILED, sqlite3_errmsg(statement->sqlite));
	}
    }

    return 1;    
}

/*
 * array = statement:fetch() 
 */
static int statement_fetch(lua_State *L) {
    return statement_fetch_impl(L, 0);
}

/*
 * hashmap = statement:fetchtable() 
 */
static int statement_fetchtable(lua_State *L) {
    return statement_fetch_impl(L, 1);
}

/*
 * __gc
 */
static int statement_gc(lua_State *L) {
    /* always free the handle */
    statement_close(L);

    return 0;
}

int dbd_sqlite3_statement_create(lua_State *L, connection_t *conn, const char *sql_query) { 
    statement_t *statement = NULL;

    statement = (statement_t *)lua_newuserdata(L, sizeof(statement_t));
    statement->sqlite = conn->sqlite;
    statement->stmt = NULL;
    statement->more_data = 0;

    if (sqlite3_prepare_v2(statement->sqlite, sql_query, strlen(sql_query), &statement->stmt, NULL) != SQLITE_OK) {
	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, sqlite3_errmsg(statement->sqlite));	
	return 2;
    } 

    luaL_getmetatable(L, DBD_SQLITE_STATEMENT);
    lua_setmetatable(L, -2);
    return 1;
} 

int dbd_sqlite3_statement(lua_State *L) {
    static const luaL_Reg statement_methods[] = {
	{"close", statement_close},
	{"execute", statement_execute},
	{"fetch", statement_fetch},
	{"fetchtable", statement_fetchtable},
	{NULL, NULL}
    };

    static const luaL_Reg statement_class_methods[] = {
	{NULL, NULL}
    };

    luaL_newmetatable(L, DBD_SQLITE_STATEMENT);
    luaL_register(L, 0, statement_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, statement_gc);
    lua_setfield(L, -2, "__gc");

    luaL_register(L, DBD_SQLITE_STATEMENT, statement_class_methods);

    return 1;    
}
