#!/usr/bin/env lua5.2

package.path = "../?.lua;" .. package.path
package.cpath = "../?.so;" .. package.cpath

require "luarocks.loader"
require 'busted.runner'()


local sql_code = {

	['encoding'] = "select %s%s as retval;",
	['select'] = "select * from select_tests where name = %s;",
	['select_multi'] = "select * from select_tests where flag = %s;",
	['select_count'] = "select count(*) as total from insert_tests;",
	['insert'] = "insert into insert_tests ( val ) values ( %s );",
	['insert_returning'] = "insert into insert_tests ( val ) values ( %s ) returning id;",
	['insert_select'] = "select * from insert_tests where id = %s;",
	['update_all'] = "update update_tests set last_update = %s;",
	['update_some'] = "update update_tests set last_update = %s where flag = %s;"
	
}


local function code( sql_test_code, arg )

	local ret = string.format(sql_code[sql_test_code], config.placeholder, arg)
	--print(ret)
	return ret

end



local function setup_tests()

	DBI = require "DBI"
	assert.is_not_nil(DBI)

	dbh, err = DBI.Connect( 
		db_type,
		config.connect.name,
		config.connect.user,
		config.connect.pass,
		config.connect.host,
		config.connect.port
	)
	
	assert.is_nil(err)
	assert.is_not_nil(dbh)
	assert.is_not_nil(dbh:autocommit( true ))
	
	assert.is_true( dbh:ping() )
	return dbh

end


local function teardown_tests()

	if dbh then
		dbh:close()
		dbh = nil
	end

end



local function connection_fail()

	local dbh2, err = DBI.Connect( 
		db_type,
		"Bogus1-3jrn23j4n32j4n23jn4@32^$2j",
		"Bogus2-#*%J#@&*FNM@JK#FM#",
		"Bogus3-#*NMR@JNM^#M<&$*%"
	)
	
	assert.is_nil(dbh2)
	assert.is_string(err)

end

local function syntax_error()


	local sth, err = dbh:prepare("Break me 3i5m2r3krm3280j648390j@#(J%28903j523890j623")
	assert.is_nil(sth)
	assert.is_string(err)

end




local function test_encoding()

	for vtype, val in pairs(config.encoding_test) do
	
		if config.have_typecasts then
			query = code('encoding', vtype)
		else
			query = code('encoding', '')
		end
		
	
		local sth = assert.is_not_nil(dbh:prepare(query))
		assert.is_not_nil(sth)
		assert.is_not_nil(sth:execute(val))
		local row = sth:rows(true)()
		assert.is_not_nil(row)
		assert.is_equal(val, row['retval'])
		assert.is_equal(type(val), type(row['retval']))
	
		sth:close()
	
	end

end


local function test_select()

	local sth, err = dbh:prepare(code('select'))
	local count = 0
	local success
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	success, err = sth:execute("Row 1")
	
	assert.is_true(success)
	assert.is_nil(err)

	for row in sth:rows(true) do
		count = count + 1
		
		if config.have_booleans then
			assert.is_true(row['flag'])
		else
			assert.equals(1, row['flag'])
		end
		
		assert.equals('Row 1', row['name'])
		assert.is_number(row['maths'])
	end

	assert.equals(1, count)
	
	if config.have_rowcount then
		assert.equals(sth:rowcount(), count)
	end
	
	sth:close()
	
end


local function test_select_multi()

	local sth, err = dbh:prepare(code('select_multi'))
	local count = 0
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	local success, err = sth:execute(false)
	
	assert.is_true(success)
	assert.is_nil(err)
	
	for row in sth:rows(false) do
		count = count + 1
		
		assert.equals(#row, 4)
		assert.is_number(row[1])
		assert.is_string(row[2])
		
		if config.have_booleans then
			assert.is_false(row[3])
		else
			assert.equals(0, row[3])
		end
		
		assert.is_number(row[4])
	end

	assert.equals(2, count)
	
	if config.have_rowcount then
		assert.equals(sth:rowcount(), count)
	end
	
	sth:close()
	
end


local function test_insert()

	local sth, sth2, err, success, stringy
	local stringy = os.date()


	sth, err = dbh:prepare(code('insert'))
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	
	success, err = sth:execute(stringy)
	
	assert.is_true(success)
	assert.is_nil(err)
	
	assert.is_equal(1, sth:affected())
	
	--
	-- Grab it back, make sure it's all good
	--
	
	local id = dbh:last_id()
	assert.is_not_nil(id)
	sth:close()
	
	sth2, err = dbh:prepare(code('insert_select'))
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	
	success, err = sth2:execute(id)
	
	assert.is_true(success)
	assert.is_nil(err)
	
	local row = sth2:rows(false)()
	assert.is_not_nil(row)
	assert.are_equal(id, row[1])
	assert.are_equal(stringy, row[2])

	sth:close()
	sth2:close()

end


local function test_insert_multi()

	local sth, sth2, err, stringy, rs, count, success
	local stringy = os.date()

	sth, err = dbh:prepare(code('insert'))
	assert.is_nil(err)
	assert.is_not_nil(sth)

	sth2, err = dbh:prepare(sql_code['select_count'])
	assert.is_nil(err)
	assert.is_not_nil(sth2)
		
	--
	-- See how many rows are in the table
	--
	rs, err = sth2:execute()
	assert.is_nil(err)
	assert.is_not_nil(rs)
	
	local itr = sth2:rows(false)
	count = itr()[1]
	
	-- drain the results - should only be one row but let's be correct
	while itr() do success = nil end
	
	--
	-- Reuse the insert statement quite a few times
	--
	for i=1,10 do 
		success, err = sth:execute(stringy .. '-' .. tostring(i))
		
		assert.is_true(success)
		assert.is_nil(err)
		assert.is_not_nil(sth)

		assert.is_equal(1, sth:affected())
	end


	--
	-- Make sure all ten inserts succeed by comparing how many
	-- rows are there now. Also proves selects are reusable.
	--
	rs, err = sth2:execute()
	assert.is_nil(err)
	assert.is_not_nil(rs)
	
	itr = sth2:rows(false)
	assert.is_equal(count + 10, itr()[1])
	while itr() do success = nil end
	
	sth:close()
	sth2:close()

end


local function test_insert_returning()

	local sth, sth2, err, success, stringy
	local stringy = os.date()


	sth, err = dbh:prepare(code('insert_returning'))
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	success, err = sth:execute(stringy)
	
	assert.is_nil(err)
	assert.is_true(success)
	
	assert.is_equal(1, sth:rowcount())

	
	--
	-- Grab it back, make sure it's all good
	--
	local id_row = sth:rows(false)()
	assert.is_not_nil(id_row)
	local id = id_row[1]
	
	assert.is_not_nil(id)
	sth:close()
	
	sth2, err = dbh:prepare(code('insert_select'))
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	success, err = sth2:execute(id)
	
	local row = sth2:rows(false)()
	assert.is_not_nil(row)
	assert.are_equal(id, row[1])
	assert.are_equal(stringy, row[2])

end


--
-- Prove affected() is functional.
--
local function test_update()

	--
	-- I originally used date handling and set the column
	-- to NOW(), but Sqlite3 didn't play nice with that.
	--

	local sth, err = dbh:prepare(code('update_all'))
	local success
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	
	success, err = sth:execute(os.time() - math.random(50, 500))
	assert.is_nil(err)
	assert.is_true(success)
	assert.equals(4, sth:affected())
	sth:close()
	sth = nil
	
	-- do it again with the flag set, so we get fewer rows,
	-- just to be sure.
	-- which also means we need to sleep for a bit.
	
	if config.have_booleans then
		sth, err = dbh:prepare(code('update_some', 'false'))
	else
		sth, err = dbh:prepare(code('update_some', '0'))
	end
		
	assert.is_nil(err)
	assert.is_not_nil(sth)
	
	success, err = sth:execute(os.time())
	assert.is_nil(err)
	assert.is_true(success)
	assert.equals(1, sth:affected())
	
end



--
-- Prove the nonexistant functions aren't there.
--
local function test_no_rowcount()

	local sth, err = dbh:prepare(code('select'))
	local success
	
	success, err = sth:execute("Row 1")
	
	assert.is_true(success)
	assert.is_nil(err)
	
	assert.has_error(function()
		local x = sth:rowcount()
	end)
	
	sth:close()

end


--
-- Prove there is no insert_id.
--
local function test_no_insert_id()

	local sth, sth2, err, success, stringy
	local stringy = os.date()


	sth, err = dbh:prepare(code('insert'))
	
	assert.is_nil(err)
	assert.is_not_nil(sth)
	success, err = sth:execute(stringy)
	
	assert.has_error(function()
		local x = dbh:insert_id()
	end)
	
	sth:close()

end


--
-- Prove something sane happens in the event that the database
-- handle goes away, but statements are still open.
--
local function test_db_close_doesnt_segfault()


	local sth, sth2, err
	sth = dbh:prepare("SELECT 1");
	
	assert.is_nil(err)
	assert.is_not_nil(sth)

	dbh:close()
	sth2, err = dbh:prepare(code('insert'))
	
	assert.is_nil(sth2)
	assert.is_string(err)
	
	
	dbh = nil
	
	assert.has_error(function()
		sth:execute()
	end)
	
	-- this also shouldn't segfault.
	sth:close()

end



describe("PostgreSQL #psql", function()
	db_type = "PostgreSQL"
	config = dofile("configs/" .. db_type .. ".lua")
	local DBI,	dbh
	
	setup(setup_tests)
	it( "Tests login failure", connection_fail )
	it( "Tests syntax error", syntax_error )
	it( "Tests value encoding", test_encoding )
	it( "Tests a simple select", test_select )
	it( "Tests multi-row selects", test_select_multi )
	it( "Tests inserts", test_insert_returning )
	it( "Tests statement reuse", test_insert_multi )
	it( "Tests no insert_id", test_no_insert_id )
	it( "Tests affected rows", test_update )
	it( "Tests closing dbh doesn't segfault", test_db_close_doesnt_segfault )
	teardown(teardown)
end)

describe("SQLite3 #sqlite3", function()
	db_type = "SQLite3"
	config = dofile("configs/" .. db_type .. ".lua")
	local DBI, dbh
	
	setup(setup_tests)
	it( "Tests syntax error", syntax_error )
	it( "Tests value encoding", test_encoding )
	it( "Tests simple selects", test_select )
	it( "Tests multi-row selects", test_select_multi )
	it( "Tests inserts", test_insert )
	it( "Tests statement reuse", test_insert_multi )
	it( "Tests no rowcount", test_no_rowcount )
	it( "Tests affected rows", test_update )
	it( "Tests closing dbh doesn't segfault", test_db_close_doesnt_segfault )
	teardown(teardown)
end)

describe("MySQL #mysql", function()
	db_type = "MySQL"
	config = dofile("configs/" .. db_type .. ".lua")
	local DBI, dbh
	
	setup(setup_tests)
	it( "Tests login failure", connection_fail )
	it( "Tests syntax error", syntax_error )
	it( "Tests value encoding", test_encoding )
	it( "Tests simple selects", test_select )
	it( "Tests multi-row selects", test_select_multi )
	it( "Tests inserts", test_insert )
	it( "Tests statement reuse", test_insert_multi )
	it( "Tests affected rows", test_update )
	it( "Tests closing dbh doesn't segfault", test_db_close_doesnt_segfault )
	teardown(teardown)
end)
