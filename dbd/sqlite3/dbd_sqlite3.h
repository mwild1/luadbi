#include <sqlite3.h>
#include <dbd/common.h>

#define DBD_SQLITE_CONNECTION	"DBD.SQLite3.Connection"
#define DBD_SQLITE_STATEMENT	"DBD.SQLite3.Statement"

typedef struct _connection {
    sqlite3 *sqlite;
} connection_t;

typedef struct _statement {
    sqlite3_stmt *stmt;
    sqlite3 *sqlite;
    int more_data;
} statement_t;

