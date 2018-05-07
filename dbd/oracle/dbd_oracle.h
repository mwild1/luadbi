#include <oci.h>
#include <dbd/common.h>

#define DBD_ORACLE_CONNECTION	"DBD.Oracle.Connection"
#define DBD_ORACLE_STATEMENT	"DBD.Oracle.Statement"

/* Oracle macros to parse version number, per 
 * https://docs.oracle.com/en/database/oracle/oracle-database/12.2/lnoci/miscellaneous-functions.html
 */
#define MAJOR_NUMVSN(v) ((sword)(((v) >> 24) & 0x000000FF)) /* version number */
#define MINOR_NUMRLS(v) ((sword)(((v) >> 20) & 0x0000000F)) /* release number */
#define UPDATE_NUMUPD(v) ((sword)(((v) >> 12) & 0x000000FF)) /* update number */
#define PORT_REL_NUMPRL(v) ((sword)(((v) >> 8) & 0x0000000F)) /* port release number */
#define PORT_UPDATE_NUMPUP(v) ((sword)(((v) >> 0) & 0x000000FF)) /* port update number */

typedef struct _bindparams {
    OCIParam *param;
    text *name;
    ub4 name_len;
    ub2 data_type;
    ub2 max_len;
    char *data;
    OCIDefine *define;
    sb2 null;
    ub2 ret_len;
    ub2 ret_err;
    ub4 csid;
    ub4 csform;
    ub2 charset;
    ub2 ncharset;
} bindparams_t;

/*
 * connection object
 */
typedef struct _connection {
    OCIEnv *oracle;
    OCISvcCtx *svc;
    OCIError *err;
    OCIServer *srv;
    OCISession *auth;
    int autocommit;
    ub2 charsetid;
    ub2 ncharsetid;
    ub4 vnum;
    ub4 prefetch_mem;
    ub4 prefetch_rows;
} connection_t;

/*
 * statement object
 */
typedef struct _statement {
    OCIStmt *stmt;
    connection_t *conn;
    int num_columns;
    bindparams_t *bind;

    int metadata;
    
    /* cache handling */
    ub4 prefetch_mem;
    ub4 prefetch_rows;
} statement_t;

