#include "dbd_db2.h"
#include "db2_common.h"

#define BIND_BUFFER_SIZE    1024

static lua_push_type_t db2_to_lua_push(unsigned int db2_type) {
    lua_push_type_t lua_type;

    switch(db2_type) {
	case SQL_BOOLEAN:
	    lua_type = LUA_PUSH_BOOLEAN;
	case SQL_SMALLINT:
	case SQL_INTEGER:
	    lua_type = LUA_PUSH_INTEGER; 
	    break;
	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
	case SQL_DECIMAL:
	    lua_type = LUA_PUSH_NUMBER;
	    break;
	    break;
	default:
	    lua_type = LUA_PUSH_STRING;
    }

    return lua_type;
}

/*
 * num_affected_rows = statement:affected()
 */
static int statement_affected(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);
    SQLINTEGER affected;

    if (!statement->stmt) {
        luaL_error(L, DBI_ERR_INVALID_STATEMENT);
    }

    (void)SQLRowCount(statement->stmt, &affected);

    lua_pushinteger(L, affected);

    return 1;
}

/*
 * free cursor and associated memory
 */
static void free_cursor(statement_t *statement) {
    if (statement->cursor_open) {
	SQLFreeStmt(statement->stmt, SQL_CLOSE);
	statement->cursor_open = 0;
    }
}

/*
 * success = statement:close()
 */
static int statement_close(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);

    free_cursor(statement);

    if (statement->resultset) {
	free(statement->resultset);
	statement->resultset = NULL;
    }

    if (statement->parambuf) {
	free(statement->parambuf);
	statement->parambuf = NULL;
    }

    statement->num_result_columns = 0;

    if (statement->stmt) {
	SQLFreeHandle(SQL_HANDLE_STMT, statement->stmt);
	statement->stmt = SQL_NULL_HSTMT;
    }

    return 0;    
}

/*
 *  column_names = statement:columns()
 */
static int statement_columns(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);

    int i;
    int d;

    if (!statement->resultset) {
	lua_pushnil(L);
	return 1;
    }

    d = 1; 
    lua_newtable(L);
    for (i = 0; i < statement->num_result_columns; i++) {
	const char *name = dbd_strlower((char *)statement->resultset[i].name);
	LUA_PUSH_ARRAY_STRING(d, name);
    }

    return 1;
}

/*
 * success = statement:execute(...)
 */
static int statement_execute(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);
    int n = lua_gettop(L);
    int p;
    int errflag = 0;
    const char *errstr = NULL;
    SQLRETURN rc = SQL_SUCCESS;
    unsigned char *buffer = statement->parambuf;
    int offset = 0;

    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];

    if (!statement->stmt) {
	lua_pushboolean(L, 0);
	lua_pushstring(L, DBI_ERR_EXECUTE_INVALID);
	return 2;
    }

    /* If the cursor is open from a previous execute, close it */
    free_cursor(statement);

    if (statement->num_params != n-1) {
        /*
	 * SQLExecute does not handle this condition,
 	 * and the client library will fill unset params
	 * with NULLs
	 */
	lua_pushboolean(L, 0);
        lua_pushfstring(L, DBI_ERR_PARAM_MISCOUNT, statement->num_params, n-1);
	return 2;
    }

    for (p = 2; p <= n; p++) {
	int i = p - 1;
	int type = lua_type(L, p);
	char err[64];
	const char *str = NULL;
	size_t len = 0;
	double *num;
	int *boolean;
	const static SQLLEN nullvalue = SQL_NULL_DATA;

	switch(type) {
	case LUA_TNIL:
	    rc = SQLBindParameter(statement->stmt, i, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)0, 0, (SQLPOINTER)&nullvalue);
	    errflag = rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
	    break;
	case LUA_TNUMBER:
	    num = (double *)(buffer + offset);
	    *num = lua_tonumber(L, p);
	    offset += sizeof(double);
	    rc = SQLBindParameter(statement->stmt, i, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 0, (SQLPOINTER)num, 0, NULL);
	    errflag = rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
	    break;
	case LUA_TSTRING:
	    str = lua_tolstring(L, p, &len);
	    rc = SQLBindParameter(statement->stmt, i, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)str, len, NULL);
	    errflag = rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
	    break;
	case LUA_TBOOLEAN:
	    boolean = (int *)(buffer + offset);
	    *boolean = lua_toboolean(L, p);
	    offset += sizeof(int);
	    rc = SQLBindParameter(statement->stmt, i, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)boolean, len, NULL);
	    errflag = rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO;
	    break;
	default:
	    /*
	     * Unknown/unsupported value type
	     */
	    errflag = 1;
            snprintf(err, sizeof(err)-1, DBI_ERR_BINDING_TYPE_ERR, lua_typename(L, type));
            errstr = err;
	}

	if (errflag)
	    break;
    }

    if (errflag) {
	lua_pushboolean(L, 0);

	if (errstr) {
	    lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, errstr);
	} else {
	    db2_stmt_diag(statement->stmt, message, sizeof(message));
	    lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, message);
	}
    
	return 2;
    }

    rc = SQLExecute(statement->stmt);
    if (rc != SQL_SUCCESS) {
	db2_stmt_diag(statement->stmt, message, sizeof(message));
	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, message);
	return 2;
    }

    statement->cursor_open = 1;

    lua_pushboolean(L, 1);
    return 1;
}



/*
 * must be called after an execute
 */
static int statement_fetch_impl(lua_State *L, statement_t *statement,
				int named_columns) {
    int i;
    int d;

    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLRETURN rc;
 
    if (!statement->cursor_open)
	goto nodata;

    /* fetch each row, and display */
    rc = SQLFetch(statement->stmt);
    if (rc == SQL_NO_DATA_FOUND) {
	free_cursor(statement);
	goto nodata;
    }

    if (rc != SQL_SUCCESS) {
	db2_stmt_diag(statement->stmt, message, sizeof(message));
        luaL_error(L, DBI_ERR_FETCH_FAILED, message);
    }

    d = 1; 
    lua_newtable(L);
    for (i = 0; i < statement->num_result_columns; i++) {
	resultset_t *rs = &statement->resultset[i];
	SQLCHAR *name = rs->name;
	lua_push_type_t lua_type = rs->lua_type;
	resultset_data_t *data = &(rs->data);

	if (rs->actual_len == SQL_NULL_DATA)
	    lua_type = LUA_PUSH_NIL;

	if (named_columns)
	    lua_pushstring(L, (const char *)name);

	switch (lua_type) {
	    case LUA_PUSH_NIL:
		lua_pushnil(L);
		break;
	    case LUA_PUSH_INTEGER:
		lua_pushinteger(L, data->integer);
		break;
	    case LUA_PUSH_NUMBER:
		lua_pushnumber(L, data->number);
		break;
	    case LUA_PUSH_BOOLEAN:
		lua_pushboolean(L, data->boolean);
		break;
	    case LUA_PUSH_STRING:
		lua_pushstring(L, (const char *)data->str);
		break;
	    default:
		luaL_error(L, DBI_ERR_UNKNOWN_PUSH);
	}

	if (named_columns)
	    lua_rawset(L, -3);
	else {
	    lua_rawseti(L, -2, d);
	    d++;
	}
    }

    return 1;    
nodata:
    lua_pushnil(L);
    return 1;
}


static int next_iterator(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, lua_upvalueindex(1), DBD_DB2_STATEMENT);
    int named_columns = lua_toboolean(L, lua_upvalueindex(2));

    return statement_fetch_impl(L, statement, named_columns);
}

/*
 * table = statement:fetch(named_indexes)
 */
static int statement_fetch(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);
    int named_columns = lua_toboolean(L, 2);

    return statement_fetch_impl(L, statement, named_columns);
}

/*
 * num_rows = statement:rowcount()
 */
static int statement_rowcount(lua_State *L) {
    luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_DB2_STATEMENT, "rowcount");

    return 0;
}

/*
 * iterfunc = statement:rows(named_indexes)
 */
static int statement_rows(lua_State *L) {
    if (lua_gettop(L) == 1) {
        lua_pushvalue(L, 1);
        lua_pushboolean(L, 0);
    } else {
        lua_pushvalue(L, 1);
        lua_pushboolean(L, lua_toboolean(L, 2));
    }

    lua_pushcclosure(L, next_iterator, 2);
    return 1;
}

/*
 * __gc
 */
static int statement_gc(lua_State *L) {
    /* always free the handle */
    statement_close(L);

    return 0;
}

/*
 * __tostring
 */
static int statement_tostring(lua_State *L) {
    statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DB2_STATEMENT);

    lua_pushfstring(L, "%s: %p", DBD_DB2_STATEMENT, statement);

    return 1;
}

int dbd_db2_statement_create(lua_State *L, connection_t *conn, const char *sql_query) { 
    SQLRETURN rc = SQL_SUCCESS;
    statement_t *statement = NULL;
    SQLHANDLE stmt;
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR *buffer = NULL;
    int buflen;
    SQLSMALLINT sqlctype;
    SQLPOINTER dataptr;
    int datalen;

    resultset_t *resultset = NULL;
    int i;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conn->db2, &stmt);
    if (rc != SQL_SUCCESS) {
	db2_dbc_diag(conn->db2, message, sizeof(message));
        lua_pushnil(L);
        lua_pushfstring(L, DBI_ERR_ALLOC_STATEMENT, message);
        return 2;
    }

    /*
     * turn off deferred prepare
     * statements will be sent to the server at prepare time,
     * and therefore we can catch errors now rather 
     * than at execute time
     */
    rc = SQLSetStmtAttr(stmt,SQL_ATTR_DEFERRED_PREPARE,(SQLPOINTER)SQL_DEFERRED_PREPARE_OFF,0);

    rc = SQLPrepare(stmt, (SQLCHAR *)sql_query, SQL_NTS);
    if (rc != SQL_SUCCESS) {
	db2_stmt_diag(stmt, message, sizeof(message));
	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, message);
	return 2;
    }

    statement = (statement_t *)lua_newuserdata(L, sizeof(statement_t));
    statement->stmt = stmt;
    statement->db2 = conn->db2;
    statement->resultset = NULL;
    statement->cursor_open = 0;
    statement->num_params = 0;
    statement->parambuf = NULL;

    /*
     * identify the number of input parameters
     */
    rc = SQLNumParams(stmt, &statement->num_params);
    if (rc != SQL_SUCCESS) {
	db2_stmt_diag(stmt, message, sizeof(message));
        lua_pushboolean(L, 0);
        lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, message);
        return 2;
    }
    if (statement->num_params > 0) {
        statement->parambuf = (unsigned char *)malloc(sizeof(double) *
						      statement->num_params);
    }

    /*
     * identify the number of output columns
     */
    rc = SQLNumResultCols(stmt, &statement->num_result_columns);

    if (statement->num_result_columns > 0) {
	resultset = (resultset_t *)malloc(sizeof(resultset_t) *
					  statement->num_result_columns);
	memset(resultset, 0, sizeof(resultset_t) *
			     statement->num_result_columns);

	buflen = 0;
	for (i = 0; i < statement->num_result_columns; i++) {
	    /*
	     * return a set of attributes for a column
	     */
	    rc = SQLDescribeCol(stmt,
                        (SQLSMALLINT)(i + 1),
                        resultset[i].name,
                        sizeof(resultset[i].name),
                        &resultset[i].name_len,
                        &resultset[i].type,
                        &resultset[i].size,
                        &resultset[i].scale,
                        NULL);

	    if (rc != SQL_SUCCESS) {
		db2_stmt_diag(stmt, message, sizeof(message));
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_DESC_RESULT, message);
		return 2;
	    }

	    resultset[i].lua_type = db2_to_lua_push(resultset[i].type);
	    if (resultset[i].lua_type == LUA_PUSH_STRING)
		buflen += (resultset[i].size + 1 + 3) & ~3;
	}

	if (buflen > 0)
	    buffer = malloc(buflen);

	for (i = 0; i < statement->num_result_columns; i++) {
	    switch (resultset[i].lua_type) {
		case LUA_PUSH_INTEGER:
		    sqlctype = SQL_C_LONG;
		    dataptr = &resultset[i].data.integer;
		    datalen = 0;
		    break;
		case LUA_PUSH_NUMBER:
		    sqlctype = SQL_C_DOUBLE;
		    dataptr = &resultset[i].data.number;
		    datalen = 0;
		    break;
		default:
		    sqlctype = SQL_C_CHAR;
		    resultset[i].data.str = buffer;
		    dataptr = buffer;
		    datalen = resultset[i].size + 1;
		    buffer += (datalen + 3) & ~3;
		    break;
	    }

	    rc = SQLBindCol(stmt,
                       (SQLSMALLINT)(i + 1),
                       sqlctype,
                       dataptr,
		       datalen,
                       &resultset[i].actual_len);

	    if (rc != SQL_SUCCESS) {
		db2_stmt_diag(stmt, message, sizeof(message));
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_ALLOC_RESULT, message);
		return 2;
	    }
	}

	statement->resultset = resultset;
    }
    luaL_getmetatable(L, DBD_DB2_STATEMENT);
    lua_setmetatable(L, -2);

    return 1;
} 

int dbd_db2_statement(lua_State *L) {
    static const luaL_Reg statement_methods[] = {
	{"affected", statement_affected},
	{"close", statement_close},
	{"columns", statement_columns},
	{"execute", statement_execute},
	{"fetch", statement_fetch},
	{"rowcount", statement_rowcount},
	{"rows", statement_rows},
	{NULL, NULL}
    };

    static const luaL_Reg statement_class_methods[] = {
	{NULL, NULL}
    };

    dbd_register(L, DBD_DB2_STATEMENT,
		 statement_methods, statement_class_methods,
		 statement_gc, statement_tostring);

    return 1;    
}
