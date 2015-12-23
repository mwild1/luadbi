# Synopsis #

```
require('DBI')

-- create a handle for an insert
local insert = assert(dbh:prepare('insert into tab(str,num,bool) values(?,?,?)'))

-- execute the handle with bind parameters
insert:execute('test', 1234, true)
insert:execute('test', 12.0, false)

-- create a select handle
local sth = assert(dbh:prepare('select str,num from tab where bool=?'))

-- execute select with a bind variable
sth:execute(true)

-- get list of column names in the result set
local columns = sth:columns()

-- iterate over the returned data
for row in sth:rows() do
    -- rows() with no arguments (or false) returns
    -- the data as a numerically indexed table
    -- passing it true returns a table indexed by
    -- column names
    print(row[1])
    print(row[2])
end

-- execute the handle again with a different bind value
sth:execute(false)

-- loop over the returned data
local row = sth:fetch(true)
while row do
    print(row.str)
    print(row.num)
    row = sth:fetch(true)
end

-- finish and close the handles
ins:close()
sth:close()

```

## Methods ##

### num\_affected\_rows = statement:affected() ###

Returns the number of rows affected by an `insert`, `update` or `delete` operation. **Does not** return the number of rows retrieved by a `select`.

### success = statement:close() ###

Closes the statement handle. Returns true if the handle was successfully closed, otherwise false.

Once a statement has been closed any further operations on that object will produce an error.

### column\_names = statement:columns() ###

Returns an ordered table of column names from the result set.

### success,err = statement:execute(...) ###

Executes the prepared statement with 0 or more bind parameters.

Returns true on success, otherwise false and an error string.

### row = statement:fetch(named\_columns) ###

Returns the next single row (as a table) in the result set of a `select` operation. If the parameter `named_colums` is true the table will be indexed with the names of the retrieved columns, otherwise numeric indexing based on column order is used.

### num\_rows = statement:rowcount() ###

Returns the number of rows retrieved after the execution of a `select` operation. **Please note** this method is not currently implemented in every database driver.

### iterator = statement:rows(named\_columns) ###

Similar to the `fetch` method but returns an iterator function (as opposed to a table) that can be used in a `for` loop.