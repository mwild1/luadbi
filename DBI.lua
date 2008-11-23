#!/usr/bin/lua

module('DBI', package.seeall)

local name_to_module = {
    MySQL = 'dbdmysql',
    PostgreSQL = 'dbdpostgresql',
    SQLite3 = 'dbdsqlite3',
}

local string = require('string')

local function available_drivers()
    local available = {}

    for driver, modulefile in pairs(name_to_module) do
	local m, err = pcall(require, modulefile)

	if m then
	    table.insert(available, driver)
	end
    end

    if table.maxn(available) < 1 then
	return '(None)'
    end

    return table.concat(available, ',')
end

function Connect(driver, name, username, password, host, port)
    local modulefile = name_to_module[driver]

    if not modulefile then
	error(string.format("Driver '%s' not found. Available drivers are: %s", driver, available_drivers()))
    end

    local m, err = pcall(require, modulefile)

    if not m then
	error(string.format('Cannot load driver %s. Available drivers are: %s', driver, available_drivers()))
    end

    local class_str = string.format('DBD.%s.Connection', driver)

    local connection_class = package.loaded[class_str]

    return connection_class.New(name, username, password, host, port)
end

