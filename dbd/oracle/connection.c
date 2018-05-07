#include "dbd_oracle.h"

int dbd_oracle_statement_create(lua_State *L, connection_t *conn, const char *sql_query);

static int commit(connection_t *conn) {
    int rc = OCITransCommit(conn->svc, conn->err, OCI_DEFAULT);
    return rc;
}

static int rollback(connection_t *conn) {
    int rc = OCITransRollback(conn->svc, conn->err, OCI_DEFAULT);
    return rc;
}


/* 
 * connection,err = DBD.Oracle.New(dbfile)
 */
static int connection_new(lua_State *L) {
    int n = lua_gettop(L);

    int rc = 0;

    const char *user = NULL;
    const char *password = NULL;
    const char *db = NULL;

    OCIEnv *env = NULL;
    OCIError *err = NULL;
    OCISvcCtx *svc = NULL;

    connection_t *conn = NULL;

    /* db, user, password */
    switch(n) {
    case 5:
    case 4:
    case 3:
	if (lua_isnil(L, 3) == 0) 
	    password = luaL_checkstring(L, 3);
    case 2:
	if (lua_isnil(L, 2) == 0) 
	    user = luaL_checkstring(L, 2);
    case 1:
        /*
         * db is the only mandatory parameter
         */
	db = luaL_checkstring(L, 1);
    }

    /*
     * initialise OCI
     */
    OCIInitialize((ub4) OCI_DEFAULT, (dvoid *)0, (dvoid * (*)(dvoid *, size_t))0, (dvoid * (*)(dvoid *, dvoid *, size_t))0, (void (*)(dvoid *, dvoid *))0);

    /*
     * initialise environment
     */
    OCIEnvNlsCreate((OCIEnv **)&env, OCI_DEFAULT, 0, NULL, NULL, NULL, 0, 0, 0, 0);

    /* 
     * server contexts 
     */
    OCIHandleAlloc((dvoid *)env, (dvoid **)&err, OCI_HTYPE_ERROR, 0, (dvoid **)0);
    OCIHandleAlloc((dvoid *)env, (dvoid **)&svc, OCI_HTYPE_SVCCTX, 0, (dvoid **)0);

    /*
     * connect to database server
     */
    rc = OCILogon(env, err, &svc, user, strlen(user), password, strlen(password), db, strlen(db));
    if (rc != 0) {
	char errbuf[100];
	int errcode;

	OCIErrorGet((dvoid *)err, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
	
	lua_pushnil(L);
	lua_pushfstring(L, DBI_ERR_CONNECTION_FAILED, errbuf);

	return 2;
    }

    /* Get the remote database version */
    text vbuf[256];
    ub4 vnum;
    rc = OCIServerRelease(svc, err, vbuf, sizeof(vbuf), OCI_HTYPE_SVCCTX, &vnum);
    
    /* Get default character set info */
    ub2 charsetid = 0;
    ub2 ncharsetid = 0;
    size_t rsize = 0;

    rc = OCINlsEnvironmentVariableGet(&charsetid,(size_t) 0, OCI_NLS_CHARSET_ID, 0, &rsize);
    rc = OCINlsEnvironmentVariableGet(&ncharsetid, (size_t) 0, OCI_NLS_NCHARSET_ID, 0, &rsize);

    conn = (connection_t *)lua_newuserdata(L, sizeof(connection_t));
    conn->oracle = env;
    conn->err = err;
    conn->svc = svc;
    conn->autocommit = 0;
    conn->charsetid = charsetid;
    conn->ncharsetid = ncharsetid;
    conn->vnum = vnum;
    conn->prefetch_mem = 1024 * 1024;
    conn->prefetch_rows = 1000000;
    
    luaL_getmetatable(L, DBD_ORACLE_CONNECTION);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * success = connection:autocommit(on)
 */
static int connection_autocommit(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
    int on = lua_toboolean(L, 2); 
    int err = 1;

    if (conn->oracle) {
	if (on)
	    rollback(conn);

	conn->autocommit = on;
	err = 0;
    }

    lua_pushboolean(L, !err);
    return 1;
}


/*
 * success = connection:close()
 */
static int connection_close(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
    int disconnect = 0;   

    if (conn->oracle) {
	rollback(conn);

	OCILogoff(conn->svc, conn->err);
	
	if (conn->svc)
	    OCIHandleFree((dvoid *)conn->svc, OCI_HTYPE_ENV);
        if (conn->err)
            OCIHandleFree((dvoid *)conn->err, OCI_HTYPE_ERROR);

	disconnect = 1;
	conn->oracle = NULL;
    }

    lua_pushboolean(L, disconnect);
    return 1;
}

/*
 * success = connection:commit()
 */
static int connection_commit(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
    int err = 1;

    if (conn->oracle) {
	err = commit(conn);
    }

    lua_pushboolean(L, !err);
    return 1;
}

/*
 * ok = connection:ping()
 */
static int connection_ping(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
    int ok = 0;

    if (conn->oracle) {
      /* Not all Oracle client libraries have OCIPing, and it's pretty
       * much impossible to tell which client version is installed
       * programmatically. Rely on a compile-time decision by the
       * operator. */
        int rc;
	if (
#ifndef ORA_ENABLE_PING
	     1 ||
#endif
	     (MAJOR_NUMVSN(conn->vnum) < 10) ||
	     (MAJOR_NUMVSN(conn->vnum) == 10 &&
	      MINOR_NUMRLS(conn->vnum) < 2) ) {
	  // client or server does not support ping; query the version number as a no-op
	  text vbuf[2]; // we're just discarding this; give it one char + a null terminator space
	  ub4 vnum;
	  rc = OCIServerRelease(conn->svc, conn->err, vbuf, sizeof(vbuf), OCI_HTYPE_SVCCTX, &vnum);
	} else {
#ifdef ORA_ENABLE_PING
	  rc = OCIPing(conn->svc, conn->err, OCI_DEFAULT);
#endif
	}
	ok = (rc == OCI_SUCCESS);
    }

    lua_pushboolean(L, ok);
    return 1;
}

/*
 * statement,err = connection:prepare(sql_str)
 */
static int connection_prepare(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);

    if (conn->oracle) {
	return dbd_oracle_statement_create(L, conn, luaL_checkstring(L, 2));
    }

    lua_pushnil(L);    
    lua_pushstring(L, DBI_ERR_DB_UNAVAILABLE);
    return 2;
}

/*
 * quoted = connection:quote(str)
 */
static int connection_quote(lua_State *L) {
    luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_ORACLE_CONNECTION, "quote");
    return 0;
}

/*
 * success = connection:rollback()
 */
static int connection_rollback(lua_State *L) {
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
    int err = 1;

    if (conn->oracle) {
	err = rollback(conn);
    }

    lua_pushboolean(L, !err);
    return 1;
}

/*
 * last_id = connection:last_id()
 */
static int connection_lastid(lua_State *L) {
    luaL_error(L, DBI_ERR_NOT_IMPLEMENTED, DBD_ORACLE_CONNECTION, "last_id");
    return 0;
}

/* 
 * version = connection:version()
 */
static int connection_version(lua_State *L) {
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
  lua_pushinteger(L, conn->vnum);

  return 1;
}

/* 
 * rows = connection:prefetch_rows(optional new_rows)
 *
 * Sets the prefetch cache row limit (in rows); default 1 million.
 * Disable by setting to 0 before creating a statement handle.
 */
static int connection_prefetch_rows(lua_State *L) {
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
  lua_pushinteger(L, conn->prefetch_rows);

  return 1;
}

/* 
 * mem = connection:prefetch_mem(optional new_mem)
 *
 * Sets the prefetch cache memory limit (in bytes); default 1024Kb.
 * Disable by setting to 0 before creating a statement handle.
 */
static int connection_prefetch_mem(lua_State *L) {
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
  lua_pushinteger(L, conn->prefetch_mem);

  return 1;
}

/* 
 * charset = connection:charset()
 */
static int connection_charset(lua_State *L) {
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);

  if (lua_gettop(L) == 2) {
    // A number may be passed in if we want to set the value
    conn->charsetid = lua_tointeger(L, 2);
  }

  lua_pushinteger(L, conn->charsetid);
  
  return 1;
}

/* 
 * ncharset = connection:ncharset()
 */
static int connection_ncharset(lua_State *L) {
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);

  if (lua_gettop(L) == 2) {
    // A number may be passed in if we want to set the value
    conn->ncharsetid = lua_tointeger(L, 2);
  }
  
  lua_pushinteger(L, conn->ncharsetid);

  return 1;
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
    connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);

    lua_pushfstring(L, "%s: %p", DBD_ORACLE_CONNECTION, conn);

    return 1;
}

int dbd_oracle_connection(lua_State *L) {
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

	// Oracle-specific methods
	{"version", connection_version},
	{"prefetch_rows", connection_prefetch_rows},
	{"prefetch_mem", connection_prefetch_mem},
	{"charset", connection_charset},
	{"ncharset", connection_ncharset},
	
	{NULL, NULL}
    };

    /*
     * class methods
     */
    static const luaL_Reg connection_class_methods[] = {
	{"New", connection_new},
	{NULL, NULL}
    };

    dbd_register(L, DBD_ORACLE_CONNECTION,
		 connection_methods, connection_class_methods,
		 connection_gc, connection_tostring);

   return 1;    
}
