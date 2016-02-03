#!/usr/bin/env lua


return {
		
	connect = {
		host = 'pgserver.zadzmo.org',
		name = 'luadbi',
		user = 'luadbi',
		pass = 'testinguser-12345!!!'
	},
	
	encoding_test = {
		['::int'] = 12435212,
		['::int'] = 463769,
		['::int'] = 8574678,
		['::int'] = -12435212,
		['::int'] = 0,
		['::int'] = 9998212,
		['::double precision'] = 123412.5235236199951,
		['::float'] = 7653.25,
		['::float'] = 7635236,
		['::float'] = 0.00,
		['::float'] = -7653.25,
		['::float'] = 1636.94783,
		['::decimal'] = 35.34,
		['::decimal'] = -3.00,
		['::decimal'] = 35848535.24,
		['::decimal'] = 0.00,
		['::boolean'] = true,
		['::boolean'] = false,
		['::varchar'] = "string 1",
		['::varchar'] = "another_string",
		['::text'] = "really long string with some escapable chars: #&*%#;@'"
	},
	
	placeholder = '$1',
	
	have_last_insert_id = false,
	have_typecasts = true,
	have_booleans = true,
	have_rowcount = true
		
}

