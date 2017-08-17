#include "dbd_db2.h"
#include "db2_common.h"

void db2_stmt_diag(SQLHANDLE stmt,
		   SQLCHAR *msg,
		   SQLINTEGER msglen) {
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;

    SQLGetDiagRec(SQL_HANDLE_STMT, stmt, 1, sqlstate, &sqlcode, msg, msglen,
		  &length);
}

void db2_dbc_diag(SQLHANDLE dbc,
		  SQLCHAR *msg,
		  SQLINTEGER msglen) {
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;

    SQLGetDiagRec(SQL_HANDLE_DBC, dbc, 1, sqlstate, &sqlcode, msg, msglen,
		  &length);
}

