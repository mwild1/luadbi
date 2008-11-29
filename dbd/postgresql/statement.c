#include "dbd_postgresql.h"

#define MAX_PLACEHOLDERS	9999 
#define MAX_PLACEHOLDER_SIZE	(1+4) /* $\d{4} */

static lua_push_type_t postgresql_to_lua_push(unsigned int postgresql_type) {
    lua_push_type_t lua_type;

    switch(postgresql_type) {
    case INT2OID:
    case INT4OID:
        lua_type =  LUA_PUSH_INTEGER;
        break;

    case FLOAT4OID:
    case FLOAT8OID:
        lua_type = LUA_PUSH_NUMBER;
        break;

    case BOOLOID:
	lua_type = LUA_PUSH_BOOLEAN;
	break;

    default:
        lua_type = LUA_PUSH_STRING;
    }

    return lua_type;
}

/*
 * replace '?' placeholders with $\d+ placeholders
 * to be compatible with PSQL API
 */
static char *replace_placeholders(lua_State *L, const char *sql) {
    size_t len = strlen(sql);
    int num_placeholders = 0;
    int extra_space = 0;
    int i;
    char *newsql;
    int newpos = 1;
    int ph_num = 1;
    int in_quote = 0;

    /*
     * dumb count of all '?'
     * this will match more placeholders than necessesary
     * but it's safer to allocate more placeholders at the
     * cost of a few bytes than risk a buffer overflow
     */ 
    for (i = 1; i < len; i++) {
	if (sql[i] == '?') {
	    num_placeholders++;
	}
    }
    
    /*
     * this is MAX_PLACEHOLDER_SIZE-1 because the '?' is 
     * replaced with '$'
     */ 
    extra_space = num_placeholders * (MAX_PLACEHOLDER_SIZE-1); 

    /*
     * allocate a new string for the converted SQL statement
     */
    newsql = malloc(sizeof(char) * (len+extra_space+1));
    memset(newsql, 0, sizeof(char) * (len+extra_space+1));
    
    /* 
     * copy first char. In valid SQL this cannot be a placeholder
     */
    newsql[0] = sql[0];

    /* 
     * only replace '?' not in a single quoted string
     */
    for (i = 1; i < len; i++) {
	/*
	 * don't change the quote flag if the ''' is preceded 
	 * bt a '\' to account for escaping
	 */
	if (sql[i] == '\'' && sql[i-1] != '\\') {
	    in_quote = !in_quote;
	}

	if (sql[i] == '?' && !in_quote) {
	    size_t n;

	    if (ph_num > MAX_PLACEHOLDERS) {
		luaL_error(L, "Sorry, you are using more than %d placeholders. Use ${num} format instead", MAX_PLACEHOLDERS);
	    }

	    n = snprintf(&newsql[newpos], MAX_PLACEHOLDER_SIZE, "$%u", ph_num++);

	    newpos += n;
	} else {
	    newsql[newpos] = sql[i];
	    newpos++;
	}
    }

    /* 
     * terminate string on the last position 
     */
    newsql[newpos] = '\0';

    /* fprintf(stderr, "[%s]\n", newsql); */
    return newsql;
}

/*
 * success = statement:close()
 */
static int statement_close(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_STATEMENT);

    if (statement->result) {
	PQclear(statement->result);
	statement->result = NULL;
    }

    return 0;    
}

/*
 * success = statement:execute(...)
 */
static int statement_execute(lua_State *L) {
    int n = lua_gettop(L);
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_STATEMENT);
    int num_bind_params = n - 1;   
    ExecStatusType status;
    int p;

    char **params;
    PGresult *result = NULL;

    statement->tuple = 0;

    params = malloc(num_bind_params * sizeof(params));

    /*
     * convert and copy parameters into a string array
     */ 
    for (p = 2; p <= n; p++) {
	int i = p - 2;	

	if (lua_isnil(L, p)) {
	    params[i] = NULL;
	} else {
	    const char *param = lua_tostring(L, p);
	    size_t len = strlen(param) + 1;

	    params[i] = malloc(len * sizeof(char));
	    memset(params[i], 0, len);

	    strncpy(params[i], param, len);
	    params[i][len] = '\0';
	}
    }

    result = PQexecPrepared(
	statement->postgresql,
        statement->name,
        num_bind_params,
        (const char **)params,
	NULL,
        NULL,
        0
    );

    /*
     * free string array
     */
    for (p = 0; p < num_bind_params; p++) {
	if (params[p]) {
	    free(params[p]);
	}
    }
    free(params);

    if (!result) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, DBI_ERR_ALLOC_RESULT,  PQerrorMessage(statement->postgresql));
	return 2;
    }
    
    status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, DBI_ERR_BINDING_EXEC, PQresultErrorMessage(result));
	return 2;
    }
    
    statement->result = result;

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * must be called after an execute
 */
static int statement_fetch_impl(lua_State *L, int named_columns) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_POSTGRESQL_STATEMENT);
    int tuple = statement->tuple++;
    int i;
    int num_columns;

    if (!statement->result) {
	luaL_error(L, DBI_ERR_FETCH_INVALID);
	return 0;
    }

    if (PQresultStatus(statement->result) != PGRES_TUPLES_OK) {
	lua_pushnil(L);
	return 1;
    }

    if (tuple >= PQntuples(statement->result)) {
	lua_pushnil(L);  /* no more results */
	return 1;
    }

    num_columns = PQnfields(statement->result);
    lua_newtable(L);
    for (i = 0; i < num_columns; i++) {
	int d = 1;
	const char *name = PQfname(statement->result, i);

	if (PQgetisnull(statement->result, tuple, i)) {
	    if (named_columns) {
		LUA_PUSH_ATTRIB_NIL(name);
            } else {
                LUA_PUSH_ARRAY_NIL(d);
            }	    
	} else {
	    const char *value = PQgetvalue(statement->result, tuple, i);
	    lua_push_type_t lua_push = postgresql_to_lua_push(PQftype(statement->result, i));

	    /*
             * data is returned as strings from PSQL
             */ 

            if (lua_push == LUA_PUSH_NIL) {
                if (named_columns) {
                    LUA_PUSH_ATTRIB_NIL(name);
                } else {
                    LUA_PUSH_ARRAY_NIL(d);
                }
            } else if (lua_push == LUA_PUSH_INTEGER) {
                int val = atoi(value);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_INT(name, val);
                } else {
                    LUA_PUSH_ARRAY_INT(d, val);
                }
            } else if (lua_push == LUA_PUSH_NUMBER) {
                double val = strtod(value, NULL);

                if (named_columns) {
                    LUA_PUSH_ATTRIB_FLOAT(name, val);
                } else {
                    LUA_PUSH_ARRAY_FLOAT(d, val);
                }
            } else if (lua_push == LUA_PUSH_STRING) {
                if (named_columns) {
                    LUA_PUSH_ATTRIB_STRING(name, value);
                } else {
                    LUA_PUSH_ARRAY_STRING(d, value);
                }
            } else if (lua_push == LUA_PUSH_BOOLEAN) {
		/* 
                 * booleans are returned as a string
                 * either 't' or 'f'
                 */
                int val = value[0] == 't' ? 1 : 0;

                if (named_columns) {
                    LUA_PUSH_ATTRIB_BOOL(name, val);
                } else {
                    LUA_PUSH_ARRAY_BOOL(d, val);
                }
            } else {
                luaL_error(L, DBI_ERR_UNKNOWN_PUSH);
            }
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

int dbd_postgresql_statement_create(lua_State *L, connection_t *conn, const char *sql_query) { 
    statement_t *statement = NULL;
    ExecStatusType status;
    PGresult *result = NULL;
    char *new_sql;
    char name[IDLEN];

    /*
     * convert SQL string into a PSQL API compatible SQL statement
     */ 
    new_sql = replace_placeholders(L, sql_query);

    snprintf(name, IDLEN, "%017u", ++conn->statement_id);

    result = PQprepare(conn->postgresql, name, new_sql, 0, NULL);

    /*
     * free converted statement after use
     */
    free(new_sql);

    if (!result) {
	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_ALLOC_STATEMENT, PQerrorMessage(statement->postgresql));
	return 2;
    }
    
    status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
	const char *err_string = PQresultErrorMessage(result);
	PQclear(result);

	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, err_string);
	return 2;
    }

    PQclear(result);

    statement = (statement_t *)lua_newuserdata(L, sizeof(statement_t));
    statement->postgresql = conn->postgresql;
    statement->result = NULL;
    statement->tuple = 0;
    strncpy(statement->name, name, IDLEN-1);
    statement->name[IDLEN-1] = '\0';

    luaL_getmetatable(L, DBD_POSTGRESQL_STATEMENT);
    lua_setmetatable(L, -2);

    return 1;
} 

int dbd_postgresql_statement(lua_State *L) {
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

    luaL_newmetatable(L, DBD_POSTGRESQL_STATEMENT);
    luaL_register(L, 0, statement_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, statement_gc);
    lua_setfield(L, -2, "__gc");

    luaL_register(L, DBD_POSTGRESQL_STATEMENT, statement_class_methods);

    return 1;    
}
