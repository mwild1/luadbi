#undef UNICODE

#include <sqlcli1.h>
#include <sqlutil.h>
#include <sqlenv.h>
#include <dbd/common.h>

#define DBD_DB2_CONNECTION   "DBD.DB2.Connection"
#define DBD_DB2_STATEMENT    "DBD.DB2.Statement"

/*
 * result set metadata
 */

typedef union _resultset_data {
    SQLCHAR *str;
    lua_Number number;
    lua_Integer integer;
    int boolean;
} resultset_data_t;

typedef struct _resultset {
    SQLSMALLINT name_len;
    SQLSMALLINT type;
    SQLUINTEGER size;
    SQLSMALLINT scale;
    SQLINTEGER actual_len;
    lua_push_type_t lua_type;
    resultset_data_t data;
    SQLCHAR name[32];
} resultset_t;

/*
 * connection object implentation
 */
typedef struct _connection {
    SQLHANDLE env;
    SQLHANDLE db2;
} connection_t;

/*
 * statement object implementation
 */
typedef struct _statement {
    resultset_t * resultset;
    unsigned char *buffer;
    SQLSMALLINT num_result_columns; /* variable for SQLNumResultCols */

    SQLHANDLE stmt;
    SQLHANDLE db2;
    int cursor_open;
    SQLSMALLINT num_params;
    unsigned char *parambuf;
} statement_t;

