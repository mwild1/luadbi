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
	
drop table if exists update_tests;
create table update_tests
	(
		id integer primary key,
		name varchar(255) not null,
		last_update int not null,
		flag bool not null
	);
	
insert into update_tests
	( 
		name, 
		last_update,
		flag
	)
	values
	(
		'Row 1',
		'now',
		1
	),
	(
		'Row 2',
		'now',
		1
	),
	(
		'Row 3',
		'now',
		0
	),
	(
		'Row 4',
		'now',
		1
	);
