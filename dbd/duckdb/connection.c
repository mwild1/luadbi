#include "dbd_duckdb.h"


int dbd_duckdb_statement_create(lua_State *L, connection_t *conn, const char *sql_query);


/*
 * connection,err = DBD.DuckDB.New(dbfile)
 */
static int connection_new(lua_State *L) {
	int n = lua_gettop(L);

	char *errmessage;
	const char *db = NULL;
	connection_t *conn = NULL;

	switch(n) {
	default:
		/*
		 * db is the only parameter for now
		 */
		db = luaL_checkstring(L, 1);
	}

	conn = (connection_t *)lua_newuserdata(L, sizeof(connection_t));
	conn->autocommit = 1;
	conn->in_transaction = 0;
	
	if (duckdb_open_ext( db, &(conn->db), NULL, &errmessage ) == DuckDBError) {
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_CONNECTION_FAILED, errmessage);
		
		duckdb_free(errmessage);
		return 2;
	}

	if (duckdb_connect( conn->db, &(conn->conn) ) == DuckDBError) {
		duckdb_close( &(conn->db) );
		
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_CONNECTION_FAILED);
		return 2;
	}
	
	luaL_getmetatable(L, DBD_DUCKDB_CONNECTION);
	lua_setmetatable(L, -2);

	return 1;
}


static int connection_prepare(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);

	if (conn->conn) {
		return dbd_duckdb_statement_create(L, conn, luaL_checkstring(L, 2));
	}

	lua_pushnil(L);
	lua_pushstring(L, DBI_ERR_DB_UNAVAILABLE);
	return 2;
}


static int connection_commit(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);
	
	if (duckdb_query(conn->conn, "COMMIT;", NULL) == DuckDBError) {
		lua_pushboolean(L, 0);
		return 0;
	}
	
	conn->in_transaction = 0;
	lua_pushboolean(L, 1);
	return 1;
}


static int connection_rollback(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);
	
	if (duckdb_query(conn->conn, "ROLLBACK;", NULL) == DuckDBError) {
		lua_pushboolean(L, 0);
		return 0;
	}
	
	conn->in_transaction = 0;
	lua_pushboolean(L, 1);
	return 1;
}


static int connection_autocommit(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);
	bool mode = lua_toboolean(L, 1);
	
	// no change
	if (conn->autocommit == mode) {
		lua_pushboolean(L, 1);
		return 1;
	}
	
	// enable autocommit while in a transaction means commit it
	if (conn->in_transaction) {
		if (mode) {
			if (duckdb_query(conn->conn, "COMMIT;", NULL) == DuckDBError) {
				lua_pushboolean(L, 0);
				return 0;
			}
			
			conn->in_transaction = 0;
		}
	}

	conn->autocommit = mode;
	lua_pushboolean(L, 1);
	return 1;
}

static int connection_ping(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);

	int ok = 1;

	if (!conn->db) {
		ok = 0;
	} else if (!conn->conn) {
		ok = 0;
	}

	lua_pushboolean(L, ok);
	return 1;
}


static int connection_close(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);
	
	duckdb_disconnect(&(conn->conn));
	conn->conn = NULL;
	
	duckdb_close(&(conn->db));
	conn->db = NULL;

	lua_pushboolean(L, 1);
	return 1;
}


/*
 * last_id = connection:last_id()
 */
static int connection_lastid(lua_State *L) {
	luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_DUCKDB_CONNECTION, "last_id");
	return 0;
}


/*
 * num_rows = statement:rowcount()
 */
static int connection_quote(lua_State *L) {
	luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_DUCKDB_CONNECTION, "quote");
	return 0;
}


/*
 * __gc
 */
static int connection_gc(lua_State *L) {
	/* always close the connection */
	connection_close(L);

	return 0;
}


/*
 * __tostring
 */
static int connection_tostring(lua_State *L) {
	connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_DUCKDB_CONNECTION);

	lua_pushfstring(L, "%s: %p", DBD_DUCKDB_CONNECTION, conn);

	return 1;
}


int dbd_duckdb_connection(lua_State *L) {
	/*
	 * instance methods
	 */
	static const luaL_Reg connection_methods[] = {
		{"autocommit", connection_autocommit},
		{"close", connection_close},
		{"commit", connection_commit},
		{"ping", connection_ping},
		{"prepare", connection_prepare},
		{"quote", connection_quote},
		{"rollback", connection_rollback},
		{"last_id", connection_lastid},
		{NULL, NULL}
	};
	
	/*
	 * class methods
	 */
	static const luaL_Reg connection_class_methods[] = {
		{"New", connection_new},
		{NULL, NULL}
	};
	
	dbd_register(L, DBD_DUCKDB_CONNECTION,
		connection_methods, connection_class_methods,
		connection_gc, connection_tostring, connection_close);
		
	return 1;
}
