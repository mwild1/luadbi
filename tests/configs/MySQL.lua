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
		463769,
		8574678,
		-12435212,
		0,
		9998212,
		7653.25,
		7635236,
		0.000,
		-7653.25,
		1636.94783,
		"string 1",
		"another_string",
		"really long string with some escapable chars: #&*%#;@'"
	},
	
	placeholder = '?',
	
	have_last_insert_id = true,
	have_typecasts = false,
	have_booleans = false,
	have_rowcount = true
	
}

