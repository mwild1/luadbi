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
		['::double precision'] = 123412.5235236199951,
		['::float'] = 7653.25,
		['::decimal'] = 35.34,
		['::boolean'] = true,
		['::boolean'] = false,
		['::varchar'] = "string 1",
		['::varchar'] = "another_string",
		['::text'] = "really long string with some escapable chars: #&*%#;@'"
	},
	
	placeholder = '$1',
	
	have_typecasts = true,
	have_booleans = true,
	have_rowcount = true
		
}

