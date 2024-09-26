CC		?= cc
AR		?= ar rcu
RANLIB		?= ranlib
RM		?= rm -rf
MKDIR		?= mkdir -p
INSTALL		?= install
INSTALL_PROGRAM	?= $(INSTALL)
INSTALL_DATA	?= $(INSTALL) -m 644
LUA_V		?= 5.4
LUA_LDIR	?= /usr/share/lua/$(LUA_V)
LUA_CDIR	?= /usr/lib/lua/$(LUA_V)

COMMON_CFLAGS	?= -g -pedantic -Wall -O2 -shared -fPIC -DPIC -std=c99
LUA_INC		?= -I/usr/include/lua$(LUA_V)
MYSQL_INC	?= -I/usr/include/mysql
PSQL_INC	?= -I/usr/include/postgresql
SQLITE3_INC	?= -I/usr/include
DUCKDB_INC	?= -I/usr/include
DB2_INC		?= -I/opt/ibm/db2exc/V9.5/include
ORACLE_INC	?= -I/usr/lib/oracle/xe/app/oracle/product/10.2.0/client/rdbms/public
CF		 = $(LUA_INC) $(COMMON_CFLAGS) $(CFLAGS) -I.

COMMON_LDFLAGS	 ?= -shared
MYSQL_LDFLAGS	?= -lmysqlclient
PSQL_LDFLAGS	?= -lpq
SQLITE3_LDFLAGS	?= -lsqlite3
DUCKDB_LDFLAGS	?= -lduckdb
DB2_LDFLAGS	?= -L/opt/ibm/db2exc/V9.5/lib64 -L/opt/ibm/db2exc/V9.5/lib32 -ldb2
ORACLE_LDFLAGS	?= -L/usr/lib/oracle/xe/app/oracle/product/10.2.0/client/lib/ -locixe
LF		 = $(COMMON_LDFLAGS) $(LDFLAGS)

MYSQL_FLAGS	 = $(CF) $(LF) $(MYSQL_INC) $(MYSQL_LDFLAGS)
PSQL_FLAGS	 = $(CF) $(LF) $(PSQL_INC) $(PSQL_LDFLAGS)
SQLITE3_FLAGS	 = $(CF) $(LF) $(SQLITE3_INC) $(SQLITE3_LDFLAGS)
DUCKDB_FLAGS	= $(CF) $(LF) $(DUCKDB_INC) $(DUCKDB_LDFLAGS)
DB2_FLAGS	 = $(CF) $(LF) $(DB2_INC) $(DB2_LDFLAGS)
ORACLE_FLAGS	 = $(CF) $(LF) $(ORACLE_INC) $(ORACLE_LDFLAGS) -DORA_ENABLE_PING -DORA_ENABLE_TAF


BUILDDIR	 = build

DBDMYSQL	 = dbd/mysql.so
DBDPSQL		 = dbd/postgresql.so
DBDSQLITE3	 = dbd/sqlite3.so
DBDDUCKDB	 = dbd/duckdb.so
DBDDB2		 = dbd/db2.so
DBDORACLE	 = dbd/oracle.so

OBJS		 = build/dbd_common.o
MYSQL_OBJS	 = $(OBJS) build/dbd_mysql_main.o build/dbd_mysql_connection.o build/dbd_mysql_statement.o
PSQL_OBJS	 = $(OBJS) build/dbd_postgresql_main.o build/dbd_postgresql_connection.o build/dbd_postgresql_statement.o
SQLITE3_OBJS	 = $(OBJS) build/dbd_sqlite3_main.o build/dbd_sqlite3_connection.o build/dbd_sqlite3_statement.o
DUCKDB_OBJS	 = $(OBJS) build/dbd_duckdb_main.o build/dbd_duckdb_connection.o build/dbd_duckdb_statement.o
DB2_OBJS	 = $(OBJS) build/dbd_db2_main.o build/dbd_db2_connection.o build/dbd_db2_statement.o
ORACLE_OBJS	 = $(OBJS) build/dbd_oracle_main.o build/dbd_oracle_connection.o build/dbd_oracle_statement.o
 
free: mysql psql sqlite3 duckdb

all:  mysql psql sqlite3 duckdb db2 oracle

mysql: $(BUILDDIR) $(MYSQL_OBJS)
	$(CC) $(MYSQL_OBJS) -o $(DBDMYSQL) $(MYSQL_FLAGS)

psql: $(BUILDDIR) $(PSQL_OBJS)
	$(CC) $(PSQL_OBJS) -o $(DBDPSQL) $(PSQL_FLAGS)

sqlite3: $(BUILDDIR) $(SQLITE3_OBJS)
	$(CC) $(SQLITE3_OBJS) -o $(DBDSQLITE3) $(SQLITE3_FLAGS)

duckdb: $(BUILDDIR) $(DUCKDB_OBJS)
	$(CC) $(DUCKDB_OBJS) -o $(DBDDUCKDB) $(DUCKDB_FLAGS)

db2: $(BUILDDIR) $(DB2_OBJS)
	$(CC) $(DB2_OBJS) -o $(DBDDB2) $(DB2_FLAGS)

oracle: $(BUILDDIR) $(ORACLE_OBJS)
	$(CC) $(ORACLE_OBJS) -o $(DBDORACLE) $(ORACLE_FLAGS)

clean:
	$(RM) $(MYSQL_OBJS) $(PSQL_OBJS) $(SQLITE3_OBJS) $(DB2_OBJS) $(ORACLE_OBJS) $(DBDMYSQL) $(DBDPSQL) $(DBDSQLITE3) $(DBDDB2) $(DBDORACLE) $(DUCKDB_OBJS)

build/dbd_common.o: dbd/common.c dbd/common.h
	$(CC) -c -o $@ $< $(CF)

build/dbd_mysql_connection.o: dbd/mysql/connection.c dbd/mysql/dbd_mysql.h dbd/common.h
	$(CC) -c -o $@ $< $(MYSQL_FLAGS)
build/dbd_mysql_main.o: dbd/mysql/main.c dbd/mysql/dbd_mysql.h dbd/common.h
	$(CC) -c -o $@ $< $(MYSQL_FLAGS)
build/dbd_mysql_statement.o: dbd/mysql/statement.c dbd/mysql/dbd_mysql.h dbd/common.h
	$(CC) -c -o $@ $< $(MYSQL_FLAGS)

build/dbd_postgresql_connection.o: dbd/postgresql/connection.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(PSQL_FLAGS)
build/dbd_postgresql_main.o: dbd/postgresql/main.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(PSQL_FLAGS)
build/dbd_postgresql_statement.o: dbd/postgresql/statement.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(PSQL_FLAGS)

build/dbd_sqlite3_connection.o: dbd/sqlite3/connection.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h 
	$(CC) -c -o $@ $< $(SQLITE3_FLAGS)
build/dbd_sqlite3_main.o: dbd/sqlite3/main.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h
	$(CC) -c -o $@ $< $(SQLITE3_FLAGS)
build/dbd_sqlite3_statement.o: dbd/sqlite3/statement.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h
	$(CC) -c -o $@ $< $(SQLITE3_FLAGS)

build/dbd_duckdb_connection.o: dbd/duckdb/connection.c dbd/duckdb/dbd_duckdb.h dbd/common.h 
	$(CC) -c -o $@ $< $(DUCKDB_FLAGS)
build/dbd_duckdb_main.o: dbd/duckdb/main.c dbd/duckdb/dbd_duckdb.h dbd/common.h
	$(CC) -c -o $@ $< $(DUCKDB_FLAGS)
build/dbd_duckdb_statement.o: dbd/duckdb/statement.c dbd/duckdb/dbd_duckdb.h dbd/common.h
	$(CC) -c -o $@ $< $(DUCKDB_FLAGS)
	
build/dbd_db2_connection.o: dbd/db2/connection.c dbd/db2/dbd_db2.h dbd/common.h 
	$(CC) -c -o $@ $< $(DB2_FLAGS)
build/dbd_db2_main.o: dbd/db2/main.c dbd/db2/dbd_db2.h dbd/common.h
	$(CC) -c -o $@ $< $(DB2_FLAGS)
build/dbd_db2_statement.o: dbd/db2/statement.c dbd/db2/dbd_db2.h dbd/common.h
	$(CC) -c -o $@ $< $(DB2_FLAGS)

build/dbd_oracle_connection.o: dbd/oracle/connection.c dbd/oracle/dbd_oracle.h dbd/common.h 
	$(CC) -c -o $@ $< $(ORACLE_FLAGS)
build/dbd_oracle_main.o: dbd/oracle/main.c dbd/oracle/dbd_oracle.h dbd/common.h
	$(CC) -c -o $@ $< $(ORACLE_FLAGS)
build/dbd_oracle_statement.o: dbd/oracle/statement.c dbd/oracle/dbd_oracle.h dbd/common.h
	$(CC) -c -o $@ $< $(ORACLE_FLAGS)

build:
	$(MKDIR) ${BUILDDIR}

install_lua:
	$(INSTALL_DATA) -D DBI.lua $(DESTDIR)$(LUA_LDIR)/DBI.lua

install_mysql: mysql install_lua
	$(INSTALL_PROGRAM) -D $(DBDMYSQL) $(DESTDIR)$(LUA_CDIR)/$(DBDMYSQL)

install_psql: psql install_lua
	$(INSTALL_PROGRAM) -D $(DBDPSQL) $(DESTDIR)$(LUA_CDIR)/$(DBDPSQL)

install_sqlite3: sqlite3 install_lua
	$(INSTALL_PROGRAM) -D $(DBDSQLITE3) $(DESTDIR)$(LUA_CDIR)/$(DBDSQLITE3)

install_duckdb: duckdb install_lua
	$(INSTALL_PROGRAM) -D $(DBDDUCKDB) $(DESTDIR)$(LUA_CDIR)/$DBDDUCKDB)

install_db2: db2 install_lua
	$(INSTALL_PROGRAM) -D $(DBDDB2) $(DESTDIR)$(LUA_CDIR)/$(DBDDB2)

install_oracle: oracle install_lua
	$(INSTALL_PROGRAM) -D $(DBDORACLE) $(DESTDIR)$(LUA_CDIR)/$(DBDORACLE)

install_free: install_lua install_mysql install_psql install_sqlite3 install_duckdb

install_all: install_lua install_mysql install_psql install_sqlite3 install_duckdb install_db2 install_oracle
