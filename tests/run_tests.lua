#!/usr/bin/env lua5.2

package.path = "../?.lua;" .. package.path
package.cpath = "../?.so;" .. package.cpath

require "luarocks.loader"
require 'busted.runner'()


local sql_code = {

	['encoding'] = "select %s%s as retval;",
	['select'] = "select * from select_tests where name = %s;",
	['select_multi'] = "select * from select_tests where flag = %s;",
	['insert'] = "insert into insert_tests ( val ) values ( %s );",
	['insert_returning'] = "insert into insert_tests ( val ) values ( %s ) returning id;",
	['insert_select'] = "select * from insert_tests where id = %s;"
	
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

	dbh:close()
	dbh = nil

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
	
	if config.has_rowcount then
		assert.is_equal(1, sth:rowcount())
	end
	
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
	
	local row = sth2:rows(false)()
	assert.is_not_nil(row)
	assert.are_equal(id, row[1])
	assert.are_equal(stringy, row[2])

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



describe("PostgreSQL", function()
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
	it( "Tests no insert_id", test_no_insert_id )
	teardown(teardown)
end)

describe("SQLite3", function()
	db_type = "SQLite3"
	config = dofile("configs/" .. db_type .. ".lua")
	local DBI, dbh
	
	setup(setup_tests)
	it( "Tests syntax error", syntax_error )
	it( "Tests value encoding", test_encoding )
	it( "Tests simple selects", test_select )
	it( "Tests multi-row selects", test_select_multi )
	it( "Tests inserts", test_insert )
	it( "Tests no rowcount", test_no_rowcount )
	teardown(teardown)
end)

describe("MySQL", function()
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
	teardown(teardown)
end)
