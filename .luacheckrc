std = "min"
files["tests"] = {
	std = "max+busted",
	new_globals = {
		"DBI",
		"dbh",
		"db_type",
		"config",
	};
}
