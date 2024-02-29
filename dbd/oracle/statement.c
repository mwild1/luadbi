#include "dbd_oracle.h"

#define ORA_US7ASCII_CSID 1
#define ORA_UTF8_CSID 871
#define ORA_AL32UTF8_CSID 873
#define ORA_UTF16_CSID 2000

#define CS_IS_UTF8( x ) ( ( (x) == ORA_UTF8_CSID ) || ( (x) == ORA_AL32UTF8_CSID ) )

#define CSFORM_IMPLIED_CSID( x,y ) ( ( (x) == SQLCS_NCHAR ) ? y->ncharsetid : y->charsetid )

#define CSFORM_IMPLIES_UTF8( x,y ) ( CS_IS_UTF8( CSFORM_IMPLIED_CSID ( x,y ) ) )
/*
 * Converts SQLite types to Lua types
 */
static lua_push_type_t oracle_to_lua_push(unsigned int oracle_type, int null) {
	lua_push_type_t lua_type;

	if (null)
		return LUA_PUSH_NIL;

	switch(oracle_type) {
	case SQLT_NUM:
	case SQLT_FLT:
		lua_type = LUA_PUSH_NUMBER;
		break;
	case SQLT_INT:
		lua_type = LUA_PUSH_INTEGER;
		break;
	default:
		lua_type = LUA_PUSH_STRING;
	}

	return lua_type;
}

/*
 * Fetch metadata from the database
 */

static void statement_fetch_metadata(lua_State *L, statement_t *statement) {
	bindparams_t *bind;
	int i;

	char errbuf[100];
	sb4 errcode;
	int rc;

	text *namep;

	if (statement->metadata)
		return;

#if 0
	// staged for later, when we'll pre-check the attribute sizes and
	// types in order to set up the prefetch cache more intelligently
	rc = OCIStmtExecute(
		statement->conn->svc,
		statement->stmt,
		statement->conn->err,
		(ub4)0,         // type == OCI_STMT_SELECT ? 0 : 1,
		(ub4)0,
		(CONST OCISnapshot *)NULL,
		(OCISnapshot *)NULL,
		OCI_DESCRIBE_ONLY
		);

	if (rc) {
		OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
		luaL_error(L, "describe %s", errbuf);
	}
#endif

	ub4 prefetch_mem = statement->prefetch_mem;
	if (prefetch_mem) {
		if (OCIAttrSet(statement->stmt, OCI_HTYPE_STMT, &prefetch_mem, sizeof(prefetch_mem),
		               OCI_ATTR_PREFETCH_MEMORY, statement->conn->err) != OCI_SUCCESS) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *)errbuf, (ub4) sizeof(errbuf), (ub4)OCI_HTYPE_ERROR);
			luaL_error(L, "prefetch_mem set %s", errbuf);
		}
	}

	ub4 prefetch_rows = statement->prefetch_rows;
	if (prefetch_rows) {
		if (OCIAttrSet(statement->stmt, OCI_HTYPE_STMT,
		               &prefetch_rows, sizeof(prefetch_rows), OCI_ATTR_PREFETCH_ROWS,
		               statement->conn->err) != OCI_SUCCESS) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *)errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
			luaL_error(L, "prefetch_rows set %s", errbuf);
		}
	}

	statement->bind = (bindparams_t *)malloc(sizeof(bindparams_t) * statement->num_columns);
	memset(statement->bind, 0, sizeof(bindparams_t) * statement->num_columns);
	bind = statement->bind;

	for (i = 0; i < statement->num_columns; i++) {
		rc = OCIParamGet(statement->stmt, OCI_HTYPE_STMT, statement->conn->err, (dvoid **)&bind[i].param, i+1);
		if (rc) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
			luaL_error(L, "param get %s", errbuf);
		}

		// NOTE: Oracle does not null terminate strings!!!
		rc = OCIAttrGet(bind[i].param, OCI_DTYPE_PARAM, (dvoid *)&(namep), (ub4 *)&(bind[i].name_len), OCI_ATTR_NAME, statement->conn->err);
		if (rc) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
			luaL_error(L, "name get %s", errbuf);
		}

		if (bind[i].name_len > DBD_ORACLE_IDENTIFIER_LEN) {
			luaL_error(L, "Oracle identifier name is too long");
		}
		memset(bind[i].name, 0, sizeof(text) * (1 + DBD_ORACLE_IDENTIFIER_LEN));
		if (bind[i].name_len) {
			strncpy((char *)bind[i].name, (const char *)namep, (size_t) bind[i].name_len);
		}

		rc = OCIAttrGet(bind[i].param, OCI_DTYPE_PARAM, (dvoid *)&(bind[i].data_type), (ub4 *)0, OCI_ATTR_DATA_TYPE, statement->conn->err);
		if (rc) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
			luaL_error(L, "datatype get %s", errbuf);
		}

		rc = OCIAttrGet(bind[i].param, OCI_DTYPE_PARAM, (dvoid *)&(bind[i].max_len), 0, OCI_ATTR_DATA_SIZE, statement->conn->err);
		if (rc) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
			luaL_error(L, "datasize get %s", errbuf);
		}

#ifdef OCI_ATTR_CHARSET_ID
		rc = OCIAttrGet(bind[i].param, OCI_DTYPE_PARAM, (dvoid *)&bind[i].csid, 0, OCI_ATTR_CHARSET_ID, statement->conn->err);
		rc = OCIAttrGet(bind[i].param, OCI_DTYPE_PARAM, (dvoid *)&bind[i].csform, 0, OCI_ATTR_CHARSET_FORM, statement->conn->err);
#endif

		switch (bind[i].data_type) {
		case SQLT_CHR: // VARCHAR2 ("character string")
			// sometimes o11 returns 0 as the length. Substitute an
			// arbitrary default.
			if (bind[i].max_len == 0) {
				bind[i].max_len = 4000;
			}
		/* Fallthrough to UTF8 handling... */

		case SQLT_AFC: // CHAR ("ANSI fixed character")
			/* If the field's charset isn't UTF-8, but the session
			 * environment implies UTF-8, then we have to account for
			 * the extra required space manually.
			 */
#ifdef OCI_ATTR_CHARSET_ID
			if (!CS_IS_UTF8(bind[i].csid) && CSFORM_IMPLIES_UTF8(bind[i].csform, statement->conn) )
#endif
			bind[i].max_len *= 4;
			break;

		case SQLT_TIMESTAMP:
		case SQLT_TIMESTAMP_TZ:
		case SQLT_TIMESTAMP_LTZ:
			// size depends on the NLS default date format. Assume a generous default.
			bind[i].max_len = 200;
			break;

		default:
			// Unhandled type...
			//	  printf("type %d [%d]\n", bind[i].data_type, bind[i].max_len);
			break;
		}

		if (bind[i].data_type == SQLT_BLOB ||
		    bind[i].data_type == SQLT_CLOB) {
			// BLOB and CLOB types are segregated because we want to
			// retrieve them as different types.  This also sets us up
			// to be able to call this with OCI_DYNAMIC_FETCH later:
			// retrieving the LOB in chunks.
			bind[i].data = calloc(bind[i].max_len, sizeof(char));
			rc = OCIDefineByPos(statement->stmt,
			                    &bind[i].define,
			                    statement->conn->err,
			                    (ub4)i+1,
			                    bind[i].data,
			                    bind[i].max_len,
			                    bind[i].data_type == SQLT_BLOB ? SQLT_BIN : SQLT_CHR,
			                    (dvoid *)&(bind[i].null),
			                    (ub2 *)&(bind[i].ret_len),
			                    (ub2 *)&(bind[i].ret_err),
			                    (ub4)OCI_DEFAULT);
		} else {
			// This is a SQLT_STR return value, which needs space for a
			// NULL terminating character.  Add 4 bytes in case it's
			// UTF-8.
			bind[i].max_len+=4;
			bind[i].data = calloc(bind[i].max_len, sizeof(char));
			rc = OCIDefineByPos(statement->stmt, &bind[i].define, statement->conn->err, (ub4)i+1, bind[i].data, bind[i].max_len, SQLT_STR, (dvoid *)&(bind[i].null), (ub2 *)&(bind[i].ret_len), (ub2 *)&(bind[i].ret_err), (ub4)OCI_DEFAULT);
		}

		if (rc) {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
			luaL_error(L, "define by pos %s", errbuf);
		}
	}

	statement->metadata = 1;
}


/*
 * num_affected_rows = statement:affected()
 */
static int statement_affected(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);
	int affected;

	if (!statement->stmt) {
		luaL_error(L, DBI_ERR_INVALID_STATEMENT);
	}

	/*
	 * get number of affected rows
	 */
	OCIAttrGet(
		(dvoid *)statement->stmt,
		(ub4)OCI_HTYPE_STMT,
		(dvoid *)&affected,
		(ub4 *)0,
		(ub4)OCI_ATTR_ROW_COUNT,
		statement->conn->err
		);

	lua_pushinteger(L, affected);

	return 1;
}

/*
 * success = statement:close()
 */
int statement_close(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);
	int ok = 0;

	if (statement->stmt) {
		OCIHandleFree((dvoid *)statement->stmt, OCI_HTYPE_STMT); /* Free handles */

		statement->stmt = NULL;
	}

	if (statement->bind) {
		free(statement->bind);
		statement->bind = NULL;
	}

	lua_pushboolean(L, ok);
	return 1;
}

/*
 *  column_names = statement:columns()
 */
static int statement_columns(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);

	int i;
	int d = 1;

	if (!statement->stmt) {
		luaL_error(L, DBI_ERR_INVALID_STATEMENT);
		return 0;
	}

	statement_fetch_metadata(L, statement);

	lua_newtable(L);
	for (i = 0; i < statement->num_columns; i++) {
		const char *name = dbd_strlower((char *)statement->bind[i].name);

		LUA_PUSH_ARRAY_STRING(d, name);
	}

	return 1;
}


/*
 * success,err = statement:execute(...)
 */
int statement_execute(lua_State *L) {
	int n = lua_gettop(L);
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);
	int p;
	int errflag = 0;
	const char *errstr = NULL;
	int num_columns;
	int rc;

	char errbuf[100];
	sb4 errcode;

	ub2 type;

	if (!statement->stmt) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, DBI_ERR_EXECUTE_INVALID);
		return 2;
	}
	for (p = 2; p <= n; p++) {
		int i = p - 1;
		int type = lua_type(L, p);
		char err[64];
		const char *value;
		size_t val_size;

		OCIBind *bnd = (OCIBind *)0;

		switch(type) {
		case LUA_TNIL:
			errflag = OCIBindByPos(
				statement->stmt,
				&bnd,
				statement->conn->err,
				i,
				NULL,
				0,
				SQLT_CHR,
				(dvoid *)0,
				(ub2 *)0,
				(ub2 *)0,
				(ub4)0,
				(ub4 *)0,
				OCI_DEFAULT);
			break;
		case LUA_TNUMBER:
		case LUA_TSTRING:
		case LUA_TBOOLEAN:
			value = lua_tolstring(L, p, &val_size);

			errflag = OCIBindByPos(
				statement->stmt,
				&bnd,
				statement->conn->err,
				(ub4)i,
				(dvoid *)value,
				(sb4)val_size,
				(ub2)SQLT_CHR,
				(dvoid *)0,
				(ub2 *)0,
				(ub2 *)0,
				(ub4)0,
				(ub4 *)0,
				(ub4)OCI_DEFAULT);
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
		if (errstr)
			lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, errstr);
		else {
			OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);

			lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, errbuf);
		}

		return 2;
	}

	/*
	 * statement type
	 */
	rc = OCIAttrGet(
		(dvoid *)statement->stmt,
		(ub4)OCI_HTYPE_STMT,
		(dvoid *)&type,
		(ub4 *)0,
		(ub4)OCI_ATTR_STMT_TYPE,
		statement->conn->err
		);

	if (rc) {
		OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);

		lua_pushboolean(L, 0);
		lua_pushfstring(L, "Error getting type: %s", errbuf);

		return 2;
	}

	/*
	 * execute statement
	 */
	rc = OCIStmtExecute(
		statement->conn->svc,
		statement->stmt,
		statement->conn->err,
		type == OCI_STMT_SELECT ? 0 : 1,
		(ub4)0,
		(CONST OCISnapshot *)NULL,
		(OCISnapshot *)NULL,
		statement->conn->autocommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT
		);

	if (rc) {
		OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);

		lua_pushboolean(L, 0);
		lua_pushfstring(L, DBI_ERR_BINDING_EXEC, errbuf);

		return 2;
	}

	/*
	 * get number of columns
	 */
	rc = OCIAttrGet(
		(dvoid *)statement->stmt,
		(ub4)OCI_HTYPE_STMT,
		(dvoid *)&num_columns,
		(ub4 *)0,
		(ub4)OCI_ATTR_PARAM_COUNT,
		statement->conn->err
		);

	if (rc) {
		OCIErrorGet((dvoid *)statement->conn->err, (ub4) 1, (text *) NULL, (sb4 *)&errcode, (text *) errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);

		lua_pushboolean(L, 0);
		lua_pushfstring(L, DBI_ERR_BINDING_PARAMS, errbuf);

		return 2;
	}

	statement->num_columns = num_columns;

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * must be called after an execute
 */
static int statement_fetch_impl(lua_State *L, statement_t *statement, int named_columns) {
	sword status;
	bindparams_t *bind;

	char errbuf[100];
	sb4 errcode;

	if (!statement->stmt) {
		luaL_error(L, DBI_ERR_FETCH_INVALID);
		return 0;
	}

	statement_fetch_metadata(L, statement);
	bind = statement->bind;

	status = OCIStmtFetch(statement->stmt, statement->conn->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT);

	if (status == OCI_NO_DATA) {
		/* No more rows */
		lua_pushnil(L);
		return 1;
	}

	// Loop through the fields, even on error; the error might be 1406 (truncated column), and we might want to return partial results...

	if (statement->num_columns) {
		int i;
		int d = 1;

		lua_newtable(L);

		for (i = 0; i < statement->num_columns; i++) {
			lua_push_type_t lua_push = oracle_to_lua_push(bind[i].data_type, bind[i].null);
			const char *name = dbd_strlower((char *)bind[i].name);
			const char *data = bind[i].data;
			size_t data_size = bind[i].ret_len;

			if ((bind[i].data_type == SQLT_BLOB ||
			     bind[i].data_type == SQLT_CLOB) &&
			    status == 1 &&
			    bind[i].ret_err == 1406) {
				// Allow partial return from a LOB
				status = 0;
			}
			else if (bind[i].ret_err != 0 &&
			         !(bind[i].ret_err == 1405 && bind[i].null)) {
				// If we need debugging...
				// printf("Error %d with column %.*s\n", bind[i].ret_err, bind[i].name_len, bind[i].name);
			}

			if (lua_push == LUA_PUSH_NIL) {
				if (named_columns) {
					LUA_PUSH_ATTRIB_NIL(name);
				} else {
					LUA_PUSH_ARRAY_NIL(d);
				}
			} else if (lua_push == LUA_PUSH_INTEGER) {
				int val = atoi(data);

				if (named_columns) {
					LUA_PUSH_ATTRIB_INT(name, val);
				} else {
					LUA_PUSH_ARRAY_INT(d, val);
				}
			} else if (lua_push == LUA_PUSH_NUMBER) {
				double val = strtod(data, NULL);

				if (named_columns) {
					LUA_PUSH_ATTRIB_FLOAT(name, val);
				} else {
					LUA_PUSH_ARRAY_FLOAT(d, val);
				}
			} else if (lua_push == LUA_PUSH_STRING) {
				if (named_columns) {
					LUA_PUSH_ATTRIB_STRING_BY_LENGTH(name, data, data_size);
				} else {
					LUA_PUSH_ARRAY_STRING_BY_LENGTH(d, data, data_size);
				}
			} else if (lua_push == LUA_PUSH_BOOLEAN) {
				int val = atoi(data);

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

	if (status != OCI_SUCCESS) {
		OCIErrorGet((dvoid *)statement->conn->err, (ub4)1, (text *)NULL, (sb4 *)&errcode, (text *) errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
		luaL_error(L, DBI_ERR_FETCH_FAILED, errbuf);
	}

	return 1;
}

static int next_iterator(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, lua_upvalueindex(1), DBD_ORACLE_STATEMENT);
	int named_columns = lua_toboolean(L, lua_upvalueindex(2));

	return statement_fetch_impl(L, statement, named_columns);
}

/*
 * table = statement:fetch(named_indexes)
 */
static int statement_fetch(lua_State *L) {
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);
	int named_columns = lua_toboolean(L, 2);

	return statement_fetch_impl(L, statement, named_columns);
}

/*
 * num_rows = statement:rowcount()
 */
static int statement_rowcount(lua_State *L) {
	luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_ORACLE_STATEMENT, "rowcount");

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
	statement_t *statement = (statement_t *)luaL_checkudata(L, 1, DBD_ORACLE_STATEMENT);

	lua_pushfstring(L, "%s: %p", DBD_ORACLE_STATEMENT, statement);

	return 1;
}

int dbd_oracle_statement_create(lua_State *L, connection_t *conn, const char *sql_query) {
	statement_t *statement = NULL;
	OCIStmt *stmt;
	char *new_sql;

	/*
	 * convert SQL string into a Oracle API compatible SQL statement
	 */
	new_sql = dbd_replace_placeholders(L, ':', sql_query);

	OCIHandleAlloc((dvoid *)conn->oracle, (dvoid **)&stmt, OCI_HTYPE_STMT, 0, (dvoid **)0);
	OCIStmtPrepare(stmt, conn->err, (CONST text *)new_sql, (ub4)strlen(new_sql), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT);

	free(new_sql);

	statement = (statement_t *)lua_newuserdata(L, sizeof(statement_t));
	statement->conn = conn;
	statement->stmt = stmt;
	statement->num_columns = 0;
	statement->bind = NULL;
	statement->metadata = 0;
	statement->prefetch_mem = conn->prefetch_mem;
	statement->prefetch_rows = conn->prefetch_rows;

	luaL_getmetatable(L, DBD_ORACLE_STATEMENT);
	lua_setmetatable(L, -2);

	return 1;
}

int dbd_oracle_statement(lua_State *L) {
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

	dbd_register(L, DBD_ORACLE_STATEMENT,
	             statement_methods, statement_class_methods,
	             statement_gc, statement_tostring);

	return 1;
}
