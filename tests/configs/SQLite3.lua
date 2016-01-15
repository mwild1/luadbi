#!/usr/bin/env lua


return {
	
	connect = {
		name = 'sqlite3-test'
	},
	
	encoding_test = {
		['::int'] = 12435212,
		['::float'] = 7653.25,
		['::decimal'] = 35.34,
		['::varchar'] = "string 1",
		['::varchar'] = "another_string",
		['::text'] = "really long string with some escapable chars: #&*%#;@'"
	},
	
	
	placeholder = '$1',
	
	have_typecasts = true,
	have_booleans = false,
	have_rowcount = false
	
}

