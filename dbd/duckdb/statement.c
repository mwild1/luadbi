#include "dbd_duckdb.h"
#include <stdio.h>


int dbd_duckdb_statement_create(lua_State *L, connection_t *conn, const char *sql_query) {
	statement_t *statement = NULL;

	statement = (statement_t *)lua_newuserdata(L, sizeof(statement_t));
	statement->conn = conn;
	statement->stmt = NULL;
	statement->is_result = 0;
	statement->cur_chunk = NULL;
	statement->cur_row = 0;

	if (duckdb_prepare(conn->conn, sql_query, &(statement->stmt) ) != DuckDBSuccess) {	
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_PREP_STATEMENT, duckdb_prepare_error( statement->stmt ));
		
		duckdb_destroy_prepare( &(statement->stmt) );
		statement->stmt = NULL;
		return 2;	
	}

	luaL_getmetatable(L, DBD_DUCKDB_STATEMENT);
	lua_setmetatable(L, -2);
	return 1;
}


/*
 * DuckDB API - the not-deprecated parts, anyway - are weird so this'll 
 * be a fun one to implement.
 */
int statement_fetch_impl(lua_State *L, statement_t *statement, int named_columns) {
	//statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);
	idx_t cols, i;
	
	
	if (!statement->stmt) {
		luaL_error(L, DBI_ERR_INVALID_STATEMENT);
		return 0;
	}
	
	if (!statement->is_result) {
		luaL_error(L, DBI_ERR_FETCH_NO_EXECUTE);
		return 0;
	}
	
	if (!statement->cur_chunk) {
		statement->cur_chunk = duckdb_fetch_chunk( statement->result );
		statement->cur_row = 0;
		
		// all data has been fetched.
		if (!statement->cur_chunk) {
			duckdb_destroy_result( &(statement->result) );
			statement->is_result = 0;
			lua_pushnil(L);
			return 1;	
		}
	}

	lua_newtable(L);
	cols = duckdb_column_count(&(statement->result));
	
	for (i = 0; i < cols; ++i) {
		duckdb_vector col1 = duckdb_data_chunk_get_vector(statement->cur_chunk, i);
		
		if (named_columns) {
			lua_pushstring(L, duckdb_column_name(&(statement->result), i));
		}
		
		uint64_t *col1_validity = duckdb_vector_get_validity(col1);
        if (duckdb_validity_row_is_valid(col1_validity, statement->cur_row)) {
			switch ( duckdb_column_type( &(statement->result), i ) ) {
  
				case DUCKDB_TYPE_TINYINT:
				case DUCKDB_TYPE_UTINYINT: {
				  	int8_t *res = duckdb_vector_get_data(col1);
				  	if (named_columns) {
						LDB_PUSH_ATTRIB_INT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_INT(i + 1, res[statement->cur_row]);
					}
				}
					break;
				
				case DUCKDB_TYPE_SMALLINT:
				case DUCKDB_TYPE_USMALLINT: {
				  	int16_t *res = duckdb_vector_get_data(col1);
				  	if (named_columns) {
						LDB_PUSH_ATTRIB_INT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_INT(i + 1, res[statement->cur_row]);
					}
				}
					break;
				
				case DUCKDB_TYPE_INTEGER:
				case DUCKDB_TYPE_UINTEGER: {
  					int32_t *res = duckdb_vector_get_data(col1);
				  	if (named_columns) {
						LDB_PUSH_ATTRIB_INT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_INT(i + 1, res[statement->cur_row]);
					}
				}
					break;
					
								
				case DUCKDB_TYPE_BIGINT:
				case DUCKDB_TYPE_UBIGINT: {
  					int64_t *res = duckdb_vector_get_data(col1);
				  	if (named_columns) {
						LDB_PUSH_ATTRIB_INT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_INT(i + 1, res[statement->cur_row]);
					}
				}
					break;

				case DUCKDB_TYPE_FLOAT: {
    				float *res = duckdb_vector_get_data(col1);	
  					if (named_columns) {
						LDB_PUSH_ATTRIB_FLOAT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_FLOAT(i + 1, res[statement->cur_row]);
					}
				}
					break;			
				
				case DUCKDB_TYPE_DOUBLE: {
    				double *res = duckdb_vector_get_data(col1);			
  					if (named_columns) {
						LDB_PUSH_ATTRIB_FLOAT( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_FLOAT(i + 1, res[statement->cur_row]);
					}
				}
					break;
  
  				case DUCKDB_TYPE_BOOLEAN: {
  					bool *res = duckdb_vector_get_data(col1);
  					if (named_columns) {
						LDB_PUSH_ATTRIB_BOOL( res[statement->cur_row] );
					} else {
  						LDB_PUSH_ARRAY_BOOL(i + 1, res[statement->cur_row]);
					}
				}
  					break;
  					
				default:
  				case DUCKDB_TYPE_BLOB:
  				case DUCKDB_TYPE_VARCHAR: {
					duckdb_string_t *vector_data = (duckdb_string_t *) duckdb_vector_get_data(col1);
					duckdb_string_t str = vector_data[ statement->cur_row ];
  					if (named_columns) {
						LDB_PUSH_ATTRIB_STRING( str );
					} else {
  						LDB_PUSH_ARRAY_STRING(i + 1, str);
					}
				}
  					break;
			}
		} else {
			// NULL value
			if (named_columns) {
				LDB_PUSH_ATTRIB_NIL();
			} else {
				LDB_PUSH_ARRAY_NIL( i + 1 );
			}
		}
	}

	++(statement->cur_row);
	
	if (statement->cur_row >= duckdb_data_chunk_get_size( statement->cur_chunk )) {
		duckdb_destroy_data_chunk(&(statement->cur_chunk));
		statement->cur_chunk = NULL;
	}
	
	return 1;
}


static int next_iterator(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, lua_upvalueindex(1), DBD_DUCKDB_STATEMENT);
	int named_columns = lua_toboolean(L, lua_upvalueindex(2));

	return statement_fetch_impl(L, statement, named_columns);
}

/*
 * table = statement:fetch(named_indexes)
 */
static int statement_fetch(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);
	int named_columns = lua_toboolean(L, 2);

	return statement_fetch_impl(L, statement, named_columns);
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


int statement_execute(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);
	int n = lua_gettop(L);
	int expected_params, p;
	int num_bind_params = n - 1;
	int errflag = 0;
	const char *errstr = NULL;
	
	
	/*
	 * Sanity checks
	 */
 	if (!statement->conn->conn) {
		luaL_error(L, DBI_ERR_DB_UNAVAILABLE);
	}

	
	/*
	 * Setup phase - Check arguments, clear handle
	 */
	expected_params = duckdb_nparams( statement->stmt );
	if (expected_params != num_bind_params) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, DBI_ERR_PARAM_MISCOUNT, expected_params, num_bind_params);
		return 2;
	}
	
	if (statement->is_result) {
		duckdb_destroy_result( &(statement->result) );
		statement->is_result = 0;
	}
	
	
	/*
	 * Bind Values
	 */
	for (p = 2; p <= n; p++) {
		int i = p - 1;
		int type = lua_type(L, p);
		char err[64];
				
				
		switch(type) {
		case LUA_TNIL:
			errflag = duckdb_bind_null( statement->stmt, i ) != DuckDBSuccess;
			break;
			
		case LUA_TNUMBER:
#if LUA_VERSION_NUM > 502
			if (lua_isinteger(L, p)) {
				errflag = duckdb_bind_int64(statement->stmt, i, lua_tointeger(L, p)) != DuckDBSuccess;
				break;
			}
#endif
			errflag = duckdb_bind_double(statement->stmt, i, lua_tonumber(L, p)) != DuckDBSuccess;
			break;
					
		case LUA_TSTRING:
			errflag = duckdb_bind_varchar(statement->stmt, i, lua_tostring(L, p)) != DuckDBSuccess;
			break;
		
		case LUA_TBOOLEAN:
			errflag = duckdb_bind_boolean(statement->stmt, i, lua_toboolean(L, p)) != DuckDBSuccess;
			break;		
		
		default:
			/*
			 * Unknown/unsupported value type
			 */
			errflag = 1;
			snprintf(err, sizeof(err)-1, DBI_ERR_BINDING_TYPE_ERR, lua_typename(L, type));
			errstr = err;
		}

		if (errflag) {
			break;
		}
	}
	
	
	/*
	 * Something went wrong, reset and return error
	 */
	if (errflag) {
		if (duckdb_clear_bindings( statement->stmt ) != DuckDBSuccess) {
			lua_pushnil(L);
			lua_pushstring(L, DBI_ERR_STATEMENT_BROKEN);
			return 2;
		}
		
		lua_pushnil(L);
		if (errstr) {
			lua_pushstring(L, errstr);
		} else {
			lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, duckdb_prepare_error( statement->stmt ));	
		}
		
		return 2;
	}
	
	
	
	/*
	 * Actual execute
	 */
	if (duckdb_execute_prepared(statement->stmt, &(statement->result)) != DuckDBSuccess) {
		// error case
		lua_pushnil(L);
		lua_pushfstring(L, DBI_ERR_EXECUTE_FAILED, duckdb_result_error( &(statement->result) ));
		duckdb_destroy_result(&(statement->result));
		return 2;
	}
			
	// success case
	statement->is_result = 1;
	lua_pushboolean(L, 1);
	return 1;
}



/*
 * success = statement:close()
 */
static int statement_close(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);
	int ok = 0;
	
	if (statement->is_result) {
		duckdb_destroy_result( &(statement->result) );
		statement->is_result = 0;
	}

	if (statement->stmt) {
		duckdb_destroy_prepare( &(statement->stmt) );
		statement->stmt = NULL;
		ok = 1;
	}

	lua_pushboolean(L, ok);
	return 1;
}


/*
 * column_names = statement:columns()
 */
static int statement_columns(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);

	int i;
	int num_columns;
	int d = 1;

	if (!statement->stmt) {
		luaL_error(L, DBI_ERR_INVALID_STATEMENT);
		return 0;
	}
	
	if (!statement->is_result) {
		luaL_error(L, DBI_ERR_FETCH_NO_EXECUTE);
		return 0;
	}

	num_columns = duckdb_column_count(&(statement->result));
	lua_newtable(L);
	for (i = 0; i < num_columns; i++) {
		const char *name = duckdb_column_name(&(statement->result), i);
		LUA_PUSH_ARRAY_STRING(d, name);
	}

	return 1;
}


/*
 * rows_changed = statement:affected()
 */
static int statement_affected(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);
	lua_pushinteger( L, duckdb_rows_changed( &(statement->result) ) );
	return 1;
}

/*
 * num_rows = statement:rowcount()
 */
static int statement_rowcount(lua_State *L) {
	/*
	 * This functionality exists in DuckDB's API but is marked deprecated 
	 * so not implementing it.
	 */
	luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_DUCKDB_STATEMENT, "rowcount");
	return 0;
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
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_DUCKDB_STATEMENT);

	lua_pushfstring(L, "%s: %p", DBD_DUCKDB_STATEMENT, statement);

	return 1;
}


int dbd_duckdb_statement(lua_State *L) {
	static const luaL_Reg statement_methods[] = {
		{"affected", statement_affected},
		{"close", statement_close},
		{"columns", statement_columns},
		{"execute", statement_execute},
		{"fetch", statement_fetch},
		{"rows", statement_rows},
		{"rowcount", statement_rowcount},
		{NULL, NULL}
	};

	static const luaL_Reg statement_class_methods[] = {
		{NULL, NULL}
	};

	dbd_register(L, DBD_DUCKDB_STATEMENT,
	             statement_methods, statement_class_methods,
	             statement_gc, statement_tostring, statement_close);

	return 1;
}
