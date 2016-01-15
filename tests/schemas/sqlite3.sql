/*

	SQLite3 test schema.

*/

drop table if exists select_tests;
create table select_tests
	(
		id integer primary key,
	
		name varchar(255) not null,
		flag boolean not null,
		
		maths int not null
	);

insert into select_tests 
	(
		name,
		flag,
		maths
	)
	values
	(
		'Row 1',
		1,
		12345
	),
	(
		'Row 2',
		0,
		54321
	),
	(
		'Row 3',
		0,
		324671
	);



drop table if exists insert_tests;
create table insert_tests
	(
		id integer primary key,
		val varchar(255) not null
	);
	
