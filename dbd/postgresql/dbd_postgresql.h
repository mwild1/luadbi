#include <libpq-fe.h>
#include <postgres.h>
#include <catalog/pg_type.h>
#include <dbd/common.h>

#define IDLEN 18

#define DBD_POSTGRESQL_CONNECTION   "DBD.PostgreSQL.Connection"
#define DBD_POSTGRESQL_STATEMENT    "DBD.PostgreSQL.Statement"

typedef struct _connection {
    PGconn *postgresql;
    unsigned int statement_id;
} connection_t;

typedef struct _statement {
    PGconn *postgresql;
    PGresult *result;
    char name[IDLEN];
    int tuple;
} statement_t;

