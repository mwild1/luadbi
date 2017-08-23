#!/usr/bin/lua

local _M = {}

-- Driver to module mapping
local name_to_module = {
    MySQL = 'dbd.mysql',
    PostgreSQL = 'dbd.postgresql',
    SQLite3 = 'dbd.sqlite3',
    DB2 = 'dbd.db2',
    Oracle = 'dbd.oracle',
    ODBC = 'dbd.odbc'
}

local string = require('string')

-- Returns a list of available drivers
-- based on run time loading
local function available_drivers()
    local available = {}

    for driver, modulefile in pairs(name_to_module) do
		local m, err = pcall(require, modulefile)

		if m then
			table.insert(available, driver)
		end
    end

    -- no drivers available
    if table.maxn(available) < 1 then
	available = {'(None)'}
    end

    return available
end

 -- High level DB connection function
 -- This should be used rather than DBD.{Driver}.New
function _M.Connect(driver, ...)
    local modulefile = name_to_module[driver]

    if not modulefile then
        local available = table.concat(available_drivers(), ',')
		error(string.format("Driver '%s' not found. Available drivers are: %s", driver, available))
    end

    local m, err = pcall(require, modulefile)

    if not m then
		-- cannot load the module, we cannot continue
        local available = table.concat(available_drivers(), ',')
		error(string.format('Cannot load driver %s. Available drivers are: %s', driver, available))
    end

    return package.loaded[modulefile].New(...)
end

-- Help function to do prepare and execute in 
-- a single step
function _M.Do(dbh, sql, ...)
    local sth,err = dbh:prepare(sql)

    if not sth then
		return false, err
    end

    local ok, err = sth:execute(...)

    if not ok then
        return false, err
    end

    return sth:affected()
end

-- List drivers available on this system
function _M.Drivers()
    return available_drivers() 
end


-- Versioning Information
_M._VERSION = '0.6'

return _M
