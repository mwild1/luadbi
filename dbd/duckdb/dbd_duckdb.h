#include <duckdb.h>
#include <dbd/common.h>

#define DBD_DUCKDB_CONNECTION   "DBD.DuckDB.Connection"
#define DBD_DUCKDB_STATEMENT    "DBD.DuckDB.Statement"

/*
 * connection object
 */
typedef struct _connection {
	duckdb_database db;
	duckdb_connection conn;
	bool autocommit;
	bool in_transaction;
} connection_t;

/*
 * statement object
 */
typedef struct _statement {
	connection_t *conn;
	duckdb_prepared_statement stmt;
	duckdb_result result;

	duckdb_data_chunk cur_chunk;
	int cur_row;
	//duckdb_vector *cur_cols;

	bool is_result;

} statement_t;


/*
 * Modified versions of what's in common.h, because DuckDB does things
 * differently.
 */
#define LDB_PUSH_ATTRIB_INT(v) \
	lua_pushinteger(L, v); \
	lua_rawset(L, -3);

#define LDB_PUSH_ATTRIB_FLOAT(v) \
	lua_pushnumber(L, v); \
	lua_rawset(L, -3);

#define LDB_PUSH_ATTRIB_STRING_BY_LENGTH(v, len) \
	lua_pushlstring(L, v, len); \
	lua_rawset(L, -3);

#define LDB_PUSH_ATTRIB_STRING(v) \
	if (duckdb_string_is_inlined(v)) {	\
		lua_pushstring(L, v.value.inlined.inlined);	\
		lua_rawset(L, -3);	\
	} else {	\
		lua_pushlstring(L, v.value.pointer.ptr, v.value.pointer.length);	\
		lua_rawset(L, -3);	\
	}

#define LDB_PUSH_ATTRIB_BOOL(v) \
	lua_pushboolean(L, v); \
	lua_rawset(L, -3);

#define LDB_PUSH_ATTRIB_NIL() \
	lua_pushnil(L); \
	lua_rawset(L, -3);



#if LUA_VERSION_NUM > 502
#define LDB_PUSH_ARRAY_INT(n, v) \
	lua_pushinteger(L, v);	\
	lua_rawseti(L, -2, n);
#else
#define LDB_PUSH_ARRAY_INT(n, v) \
	lua_pushnumber(L, v);	\
	lua_rawseti(L, -2, n);
#endif

#define LDB_PUSH_ARRAY_FLOAT(n, v) \
	lua_pushnumber(L, v); \
	lua_rawseti(L, -2, n);

// DuckDB stores strings in two different ways, compensate for that here
#define LDB_PUSH_ARRAY_STRING(n, v) \
	if (duckdb_string_is_inlined(v)) {	\
		lua_pushstring(L, v.value.inlined.inlined);	\
		lua_rawseti(L, -2, n);	\
	} else {	\
		lua_pushlstring(L, v.value.pointer.ptr, v.value.pointer.length);	\
		lua_rawseti(L, -2, n);	\
	}

#define LDB_PUSH_ARRAY_BOOL(n, v) \
	lua_pushboolean(L, v); \
	lua_rawseti(L, -2, n);

#define LDB_PUSH_ARRAY_NIL(n) \
	lua_pushnil(L); \
	lua_rawseti(L, -2, n);
