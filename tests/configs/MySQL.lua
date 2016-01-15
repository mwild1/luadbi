#!/usr/bin/env lua


return {

	connect = {
		host = 'mysqlserver.zadzmo.org',
		port = 3306,
		name = 'luadbi',
		user = 'luadbi',
		pass = 'testing12345!!!'
	},
	
	encoding_test = {
		12435212,
		7653.25,
		35.34,
		"string 1",
		"another_string",
		"really long string with some escapable chars: #&*%#;@'"
	},
	
	placeholder = '?',
	
	have_typecasts = false,
	have_booleans = false,
	have_rowcount = true
	
}

