#include <mysql.h>
#include <dbd/common.h>

#define DBD_MYSQL_CONNECTION	"DBD.MySQL.Connection"
#define DBD_MYSQL_STATEMENT	"DBD.MySQL.Statement"

typedef struct _connection {
    MYSQL *mysql;
} connection_t;

typedef struct _statement {
    MYSQL *mysql;
    MYSQL_STMT *stmt;
    MYSQL_RES *metadata;
} statement_t;

