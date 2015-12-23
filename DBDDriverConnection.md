# Synopsis #

```
require('DBI')

-- Create a connection
local dbh = assert(DBI.Connect('Driver', db, username, password, host, port))

-- set the autocommit flag
-- this is turned off by default
dbh:autocommit(true)

-- check status of the connection
local alive = dbh:ping()

-- prepare a connection
local sth = assert(dbh:prepare(sql_string))

-- commit the transaction
dbh:commit()

-- finish up
local ok = dbh:close()

```



## Methods ##

### alive = connection:ping() ###

Checks the health of the underlying connection to the database. Returns true if the connection is still valid, otherwise false.

### success = connection:autocommit(turn\_on) ###

Turns on/off autocommit for this connection handle. If autocommit is turned on all updates to the database are done immediately. By default autocommit is turned off.

### ok = connection:close() ###

Closes the connection to database. Returns true if the connection was successfully closed, otherwise false. All transactions currently in progress at the time close is called on the connection object are rolled back.

Once a connection has been closed any further operations on that connection object will produce an error.

### success = connection:commit() ###

Commits all database activity since the last `commit` or `rollback`.

### statement\_handle,err = connection:prepare(sql\_string) ###

Creates a statement handle from the SQL statement passed as a parameter. Returns an instance of DBD.Driver.Statement on success, and nil and an error string on error.

See PreparedStatementsAndBindVariables for more information.

### quoted = connection:quote(str) ###

Escapes characters in the parameter `str` to be safe for SQL statement parsing. Please note it is always safer to use bind parameters for your data.

### success = connection:rollback() ###

Rolls back all database activity since the last `commit` or `rollback`