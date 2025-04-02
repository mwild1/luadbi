#!/usr/bin/env lua


return {

	connect = {
		name = 'duckdb-test'
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
		true,
		false,
		nil,
		"string 1",
		"another_string",
		"really long string with some escapable chars: #&*%#;@'"
	},

	placeholder = '$1',

	have_last_insert_id = false,
	have_typecasts = false,
	have_booleans = true,
	have_rowcount = false

}

