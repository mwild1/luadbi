# Tutorial #

This document shows a very simple LuaDBI program, with notes, to
explain the basic workflow. It assumes that you already have a
working database.

This document will use PostgreSQL for the driver where it must be
shown, obviously this will differ depending on the database you use.

This document is very basic and intended for beginners. Users 
familiar with other database access layers (such as Perl's DBI 
module) might find this all very familiar.

### Install ###

The usual quickest way to install LuaDBI is via Luarocks. You will
need the base 'DBI' Rock, and well as the appropriate 'DBD' driver:

	# luarocks install luadbi
	# luarocks install luadbi-postgresql
	

Other install methods are possible depending on your taste and
operating system.


### Connecting to the Database ###

Let's get to the code: it is possible to jump directly to the DBD
module, but it is better to go through the top level DBI. If done
you will not need to ``require()`` anything else:

	DBI = require "DBI"
  
 
With DBI loaded, time to connect:

	dbd, err = DBI.Connect( 'PostgreSQL', 'your_database', 'dbuser', 'password' )
	assert(dbd, err)
  
  
For simple programs, Autocommit mode might be desirable. It defaults to
off. Turning it on causes LuaDBI to wrap every database call in it's
own transaction, and commit it immediately if there are no errors.

	dbd:autocommit(true)


### Preparing statements ###

LuaDBI is based around a prepare / execute / fetch loop. Calling
prepare creates a statement object:

	statement, err = dbd:prepare( "select * from table_1 where id = $1;" )
	assert(statement, err)
  
Note the '$1'. This is a placeholder - you may call the statement
object multiple times and put a different value in each time. Again,
this syntax is PostgreSQL-specific. Check your database documentation
for details if using something different.

If there is a syntax error in your SQL code, it will cause an error 
here. The prepare method will return nil and an error message.


### Execute the statement ###

To actually call the database, the statement object has an ``execute``
method:

	statement:execute( 15 )
  
If not using any placeholders, you do not need to provide any arguments.
In this tutorial here, we are using a placeholder, so it was passed in.
``select * from table_1 where id = 15;`` has just been executed.


### Retrieving data ###

A fast, Lua-ish way to step through the data is to simply call
``statement:rows()``. This gives you an iterator that works like ``pairs()``
or ``ipairs()``.

Each pass through the loop, ``rows()`` returns a single table, which
contains all the data in that row. It also takes one argument - if true,
the indexes of the tables returned will be set to the column names.

	for row in statement:rows(true) do
  
		print(row['id'])
		print(row['column1'])
		print(row['column2'])
		print(row['column3'])
  
	end


Once all data has been retrieved, you may call ``statement:execute()``
again, with different parameters if needed.
  
  
### Shortcuts ###

If you aren't worried about performance, and don't need to retrieve
data - just insert or delete something - you can skip the prepare /
execute / fetch loop:

	DBI.Do( dbd, "delete from table_1 where id = 200;" )
  

### Full Program ###

This puts most of the above together into one program, to see how it
looks as functional code:

	#!/usr/bin/env lua
	DBI = require "DBI"

	dbd, err = DBI.Connect( 'PostgreSQL', 'your_database', 'dbuser', 'password' )
	assert(dbd, err)
  
	dbd:autocommit(true)
  
	statement = dbd:prepare( "select * from table_1 where id = $1;" )
	statement:execute( 15 )
  
	for row in statement:rows(true) do
  
		print(row['column1'])
		print(row['column2'])
		print(row['column3'])
  
	end
  
  
While only showing ``SELECT`` statements, ``INSERT``, ``UPDATE``, 
and ``DELETE`` work exactly the same way, except ``rows()`` and the 
other methods for retrieving data will return nothing.
  
This should be enough to write usable programs using LuaDBI. Check
the documentation for more advanced features.


   
