#include "dbd_oracle.h"

struct _oracleconst {
  const char *name;
  int val;
};
static const struct _oracleconst constants[] = { 
  { "OCI_FO_END",     OCI_FO_END     },
  { "OCI_FO_ABORT",   OCI_FO_ABORT   },
  { "OCI_FO_REAUTH",  OCI_FO_REAUTH  },
  { "OCI_FO_BEGIN",   OCI_FO_BEGIN   },
  { "OCI_FO_ERROR",   OCI_FO_ERROR   },
  { "OCI_FO_RETRY",   OCI_FO_RETRY   },
  { "OCI_FO_NONE",    OCI_FO_NONE    },
  { "OCI_FO_SESSION", OCI_FO_SESSION },
  { "OCI_FO_SELECT",  OCI_FO_SELECT  },
  { "OCI_FO_TXNAL",   OCI_FO_TXNAL   },
  { NULL,             0              }
};

int dbd_oracle_statement_create(lua_State *L, connection_t *conn, const char *sql_query);

static int commit(connection_t *conn) {
    int rc = OCITransCommit(conn->svc, conn->err, OCI_DEFAULT);
    return rc;
}

static int rollback(connection_t *conn) {
    int rc = OCITransRollback(conn->svc, conn->err, OCI_DEFAULT);
    return rc;
}

#ifdef ORA_ENABLE_TAF
static sb4 taf_cbk(svchp, envhp, fo_ctx, fo_type, fo_event )
     dvoid * svchp;
     dvoid * envhp;
     dvoid *fo_ctx;
     ub4 fo_type;
     ub4 fo_event;
{
  /* fo_ctx is our 'conn' struct. */
  connection_t *conn = (connection_t *)fo_ctx;

  /* If we haven't registered a callback, then we're done. */
  if (!conn->cbfuncidx)
    return 0;

  /* Otherwise, call the callback with whatever we're being told; get
   * the return value; and process it appropriately. We expect a
   * single boolean return value, which defines whether or not we
   * should interrupt the normal flow. Returning a false value will
   * continue normally; returning a true value will cause us to return 
   * OCI_FO_RETRY (currently, the only interrupt flag that the Oracle 
   * API supports).
   *
   * We pass these integer arguments to the callback function:
   *   - fo_type (OCI_FO_NONE, OCI_FO_SESSION, OCI_FO_SELECT, OCI_FO_TXNAL
   *   - fo_event (OCI_FO_BEGIN, OCI_FO_ABORT, OCI_FO_END, OCI_FO_REAUTH
   * And an optional callback context argument, if one was provided.
   *
   * We expect one boolean in return. If it returns no values at all,
   * we'll raise an exception.
   */

  lua_State *L = conn->L;

  /* Find the callback function and set up its arguments */
  lua_rawgeti(L, LUA_REGISTRYINDEX, conn->cbfuncidx);
  lua_pushinteger(L, fo_type);
  lua_pushinteger(L, fo_event);
  if (conn->cbargidx) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, conn->cbargidx);
  } else {
    lua_pushnil(L);
  }

  lua_call(L, 3, 1);
  int retval = lua_toboolean(L, -1);
  lua_pop(L, 1);

  if (retval)
    return OCI_FO_RETRY;

  return 0;
}
#endif

static void RaiseErrorOnFailure(lua_State *L, int statusCode, OCIError *errhp, const char *msg)
{
  char errbuf[100];
  int errcode;

  switch (statusCode) {
  case OCI_SUCCESS:
  case OCI_SUCCESS_WITH_INFO:
    return;
  case OCI_NEED_DATA:
    strcpy(errbuf, "OCI_NEED_DATA");
    break;
    case OCI_NO_DATA:
      strcpy(errbuf, "OCI_NO_DATA");
      break;
    case OCI_ERROR:
      OCIErrorGet ((dvoid *) errhp, (ub4) 1, (text *) NULL, &errcode,
		   (text *)errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
      break;
    case OCI_INVALID_HANDLE:
      strcpy(errbuf, "OCI_INVALID_HANDLE");
      break;
    case OCI_STILL_EXECUTING:
      strcpy(errbuf, "OCI_STILL_EXECUTE");
      break;
    case OCI_CONTINUE:
      strcpy(errbuf, "OCI_CONTINUE");
      break;
    default:
      strcpy(errbuf, "Unknown OCI error");
      break;
    }

  luaL_error(L, "%s: %s", msg, errbuf);
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

    /* This code leaks many of these if there are errors setting up
     * the initial connection. FIXME: address this. */
    OCIEnv *env = NULL;
    OCIError *err = NULL;
    OCISvcCtx *svc = NULL;
    OCIServer *svr = NULL;
    OCISession *sess = NULL;

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
     *
     * Much of this connection mechanism comes directly from oracle:
     *   http://www.oracle.com/technetwork/database/windows/fscawp32-130265.pdf
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
    OCIHandleAlloc((dvoid *)env, (dvoid **)&svr, OCI_HTYPE_SERVER, 0, (dvoid **)0);
    OCIAttrSet(svc, OCI_HTYPE_SVCCTX, svr, (ub4) 0, OCI_ATTR_SERVER, err);

    
    OCIHandleAlloc((dvoid *)env, (dvoid**)&sess, OCI_HTYPE_SESSION, 0, (dvoid **)0);
    rc = OCIAttrSet((dvoid *)sess, (ub4)OCI_HTYPE_SESSION, (dvoid *)user, (ub4)strlen(user), (ub4) OCI_ATTR_USERNAME, err);
    RaiseErrorOnFailure(L, rc, err, "setting username");

    rc = OCIAttrSet((dvoid *)sess, (ub4)OCI_HTYPE_SESSION, (dvoid *)password, (ub4)strlen(password), (ub4) OCI_ATTR_PASSWORD, err);
    RaiseErrorOnFailure(L, rc, err, "setting password");

    rc = OCIServerAttach((OCIServer *)svr, (OCIError *)err, (CONST text *)db, (sb4)strlen(db), (ub4)0);
    RaiseErrorOnFailure(L, rc, err, "setting server instance");

    rc = OCISessionBegin(svc, err, sess, OCI_CRED_RDBMS, (ub4) OCI_DEFAULT);
    RaiseErrorOnFailure(L, rc, err, "logging in");

    rc = OCIAttrSet(svc, OCI_HTYPE_SVCCTX, sess, (ub4) 0, OCI_ATTR_SESSION, err);
    if (rc != OCI_SUCCESS) {
      char errbuf[100];
      int errcode;
      OCIErrorGet ((dvoid *) err, (ub4) 1, (text *) NULL, (sb4 *)&errcode,
		   (text *)errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);

      lua_pushnil(L);
      lua_pushfstring(L, DBI_ERR_CONNECTION_FAILED, errbuf);
      
      return 2;
    }

    /* We'll need the connection info for the TAF callback, so the 
     * struct needs to be created here and initialized, in case TAF 
     * gets invoked during login.
     */
    conn = (connection_t *)lua_newuserdata(L, sizeof(connection_t));
    conn->oracle = env;
    conn->err = err;
    conn->svc = svc;
    conn->svr = svr;
    conn->autocommit = 0;
    conn->prefetch_mem = 1024 * 1024;
    conn->prefetch_rows = 1000000;
    conn->cbfuncidx = 0;

#ifdef ORA_ENABLE_TAF
    /* Register a callback for TAF events. This is a stub, so the user
       doesn't have to set the callback until after they've connected
       (the Oracle API requires that we set it before we connect). If
       the app doesn't support TAF ... well, I might have to rethink
       this. */
    OCIFocbkStruct FailoverInfo;
    FailoverInfo.fo_ctx = conn;
    FailoverInfo.callback_function = taf_cbk;
    rc = OCIAttrSet(svr, (ub4) OCI_HTYPE_SERVER, (dvoid *) &FailoverInfo, 
		    (ub4) 0, OCI_ATTR_FOCBK, err);
    RaiseErrorOnFailure(L, rc, err, "registering TAF callback");
#endif
    conn->L = L;

    /* Get the remote database version */
    text vbuf[256];
    ub4 vnum;
    rc = OCIServerRelease(svc, err, vbuf, sizeof(vbuf), OCI_HTYPE_SVCCTX, &vnum);
    conn->vnum = vnum;
    
    /* Get default character set info */
    size_t rsize = 0;
    rc = OCINlsEnvironmentVariableGet(&conn->charsetid,(size_t) 0, OCI_NLS_CHARSET_ID, 0, &rsize);
    rc = OCINlsEnvironmentVariableGet(&conn->ncharsetid, (size_t) 0, OCI_NLS_NCHARSET_ID, 0, &rsize);
    
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
	if (conn->srv)
	  OCIHandleFree((dvoid *)conn->svr, OCI_HTYPE_SERVER);

	if (conn->cbfuncidx)
	  luaL_unref(L, LUA_REGISTRYINDEX, conn->cbfuncidx);

	if (conn->cbargidx) 
	  luaL_unref(L, LUA_REGISTRYINDEX, conn->cbargidx);

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
  if (lua_gettop(L) == 2) {
    // A number may be passed in if we want to set the value
    conn->prefetch_rows = lua_tointeger(L, 2);
  }

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
  if (lua_gettop(L) == 2) {
    // A number may be passed in if we want to set the value
    conn->prefetch_mem = lua_tointeger(L, 2);
  }

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

/* 
 * bool = connection:taf_available()
 */
static int connection_taf_available(lua_State *L) {
#ifdef ORA_ENABLE_TAF
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
  int rc = -1;
  ub4 can_taf = 0xFFFF;
  ub4 ct_size = sizeof(can_taf);

  if (conn->oracle) {
    rc = OCIAttrGet((CONST dvoid *)conn->svr, (ub4)OCI_HTYPE_SERVER, 
		    (dvoid *)&can_taf,
		    (ub4 *)&ct_size,
		    (ub4)OCI_ATTR_TAF_ENABLED,
		    (OCIError *)conn->err);
    RaiseErrorOnFailure(L, rc, conn->err, "checking TAF");
  }

  lua_pushboolean(L, (rc == OCI_SUCCESS) && can_taf);
#else
  lua_pushboolean(L, 0);
#endif

  return 1;
}

/* 
 * connection:taf_callback(cb, arg)
 *
 *   sets a callback for Oracle Transparent Application Failover events.
 *   'arg' is optional, but is passed to the callback if provided.
 */
static int connection_taf_callback(lua_State *L) {
#ifdef ORA_ENABLE_TAF
  connection_t *conn = (connection_t *)luaL_checkudata(L, 1, DBD_ORACLE_CONNECTION);
  if ((lua_gettop(L) != 2 && lua_gettop(L) != 3) ||
      !lua_isfunction(L, 2)) {
    luaL_error(L, "Wrong number or incorrect type of arguments to taf_callback(func, arg)");
  }

  /* Release any previous callback if we had one */
  if (conn->cbfuncidx) {
    luaL_unref(L, LUA_REGISTRYINDEX, conn->cbfuncidx);
    conn->cbfuncidx = 0;
  }

  if (conn->cbargidx) {
    luaL_unref(L, LUA_REGISTRYINDEX, conn->cbargidx);
    conn->cbargidx = 0;
  }

  /* Store the callback in the global Lua registry */
  lua_pushvalue(L, 2);
  conn->cbfuncidx = luaL_ref(L, LUA_REGISTRYINDEX);
  conn->L = L;

  if (lua_gettop(L) == 3) {
    /* Store the callback argument too */
    lua_pushvalue(L, 3);
    conn->cbargidx = luaL_ref(L, LUA_REGISTRYINDEX);
  }
#endif

  return 0;
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
	{"taf_available", connection_taf_available},
	{"taf_callback", connection_taf_callback},
	
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

    /* Register Oracle constants in our new metatable */
    luaL_getmetatable(L, DBD_ORACLE_CONNECTION);
    const struct _oracleconst *p = constants;
    while (p->name) {
      lua_pushstring(L, p->name);
      lua_pushinteger(L, p->val);
      lua_rawset(L, -3);
      p++;
    }
    lua_pop(L, 1);

   return 1;    
}
