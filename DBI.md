# Synopsis #

```
require('DBI')

local dbh = assert(DBI.Connect(driver_name, dbname, dbuser, dbpassword, dbhost, dbport))

local affected_rows = DBI.Do(dbh, "DELETE FROM tab WHERE id < ?", 100)

```

The DBI package is the main entry point into LuaDBI. Rather than calling a `DBI.Driver.Connect` method directly, the `DBI.Connect` method should be used.

## Methods ##

### dbh, err = DBI.Connect(driver\_name, dbname, dbuser, dbpassword, dbhost, dbport) ###

Connects to the database `dbname` of type `driver_name` on host `dbhost:dbport` using `dbuser` and `dbpass` for authentication. If the connection was successful, a database connection object is returned, otherwise nil and an error string is returned.

#### Default values ####

The only required parameters for the `DBI.Connect` method are `driver_name` and `dbname`. For the other parameters default values (based on the database driver implementation) are used.

#### Connection failures ####

Database connections can fail because:

  * A database error occurred, such as a failure to authenticate with the provided username/password
  * The database driver type requested is not currently installed

#### Drivers ####

Currently supported values for `driver_name` are:

  * MySQL
  * PostgreSQL
  * SQLite3
  * DB2
  * Oracle

### num\_rows\_affected, error\_string = DBI.Do(sql, ...) ###

The `Do` method is a shortcut for simple database operations. It combines the `prepare` and `execute` steps for an SQL statement. Returns the number of rows affected by the operation on success, otherwise false and an error string.