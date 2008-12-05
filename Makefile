CC=gcc
CFLAGS=-g -pedantic -O2 -Wall -shared -fpic -I /usr/include/lua5.1 -I /usr/include/mysql -I /usr/include/postgresql/ -I /opt/ibm/db2exc/V9.5/include/ -I . 
AR=ar rcu
RANLIB=ranlib
RM=rm -f

COMMON_LDFLAGS=
MYSQL_LDFLAGS=$(COMMON_LDFLAGS) -lmysqlclient
PSQL_LDFLAGS=$(COMMON_LDFLAGS) -lpq 
SQLITE3_LDFLAGS=$(COMMON_LDFLAGS) -lsqlite3 
DB2_LDFLAGS=$(COMMON_LDFLAGS) -L/opt/ibm/db2exc/V9.5/lib32 -ldb2 

DBDMYSQL=dbdmysql.so
DBDPSQL=dbdpostgresql.so
DBDSQLITE3=dbdsqlite3.so
DBDDB2=dbddb2.so

MYSQL_OBJS=build/dbd_mysql_main.o build/dbd_mysql_connection.o build/dbd_mysql_statement.o
PSQL_OBJS=build/dbd_postgresql_main.o build/dbd_postgresql_connection.o build/dbd_postgresql_statement.o
SQLITE3_OBJS=build/dbd_sqlite3_main.o build/dbd_sqlite3_connection.o build/dbd_sqlite3_statement.o
DB2_OBJS=build/dbd_db2_main.o build/dbd_db2_connection.o build/dbd_db2_statement.o

free: dbdmysql  dbdpsql dbdsqlite3

all:  dbdmysql	dbdpsql	dbdsqlite3 dbddb2

dbdmysql: $(MYSQL_OBJS)
	$(CC) $(CFLAGS) $(MYSQL_OBJS) -o $(DBDMYSQL) $(MYSQL_LDFLAGS)

dbdpsql: $(PSQL_OBJS)
	$(CC) $(CFLAGS) $(PSQL_OBJS) -o $(DBDPSQL) $(PSQL_LDFLAGS)

dbdsqlite3: $(SQLITE3_OBJS)
	$(CC) $(CFLAGS) $(SQLITE3_OBJS) -o $(DBDSQLITE3) $(SQLITE3_LDFLAGS)

dbddb2: $(DB2_OBJS)
	$(CC) $(CFLAGS) $(DB2_OBJS) -o $(DBDDB2) $(DB2_LDFLAGS)

clean:
	$(RM) $(MYSQL_OBJS) $(PSQL_OBJS) $(SQLITE3_OBJS) $(DB2_OBJS) $(DBDMYSQL) $(DBDPSQL) $(DBDSQLITE3) $(DBDDB2)

build/dbd_mysql_connection.o: dbd/mysql/connection.c dbd/mysql/dbd_mysql.h dbd/common.h 
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_mysql_main.o: dbd/mysql/main.c dbd/mysql/dbd_mysql.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_mysql_statement.o: dbd/mysql/statement.c dbd/mysql/dbd_mysql.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)

build/dbd_postgresql_connection.o: dbd/postgresql/connection.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_postgresql_main.o: dbd/postgresql/main.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_postgresql_statement.o: dbd/postgresql/statement.c dbd/postgresql/dbd_postgresql.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)

build/dbd_sqlite3_connection.o: dbd/sqlite3/connection.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h 
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_sqlite3_main.o: dbd/sqlite3/main.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_sqlite3_statement.o: dbd/sqlite3/statement.c dbd/sqlite3/dbd_sqlite3.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)

build/dbd_db2_connection.o: dbd/db2/connection.c dbd/db2/dbd_db2.h dbd/common.h 
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_db2_main.o: dbd/db2/main.c dbd/db2/dbd_db2.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)
build/dbd_db2_statement.o: dbd/db2/statement.c dbd/db2/dbd_db2.h dbd/common.h
	$(CC) -c -o $@ $< $(CFLAGS)

