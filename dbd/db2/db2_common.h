#ifndef DBD_DB2_COMMON_H
#define DBD_DB2_COMMON_H

void db2_stmt_diag(SQLHANDLE stmt,
		   SQLCHAR *msg,
		   SQLINTEGER msglen);

void db2_dbc_diag(SQLHANDLE dbc,
		  SQLCHAR *msg,
		  SQLINTEGER msglen);

#endif /* DBD_DB2_COMMON_H */
