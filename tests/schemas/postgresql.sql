/*

	PostgreSQL test schema.

*/

drop table if exists select_tests cascade;
create table select_tests
	(
		id serial primary key,
	
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
		true,
		12345
	),
	(
		'Row 2',
		false,
		54321
	),
	(
		'Row 3',
		false,
		324671
	);

grant select on select_tests to luadbi;



drop table if exists insert_tests cascade;
create table insert_tests
	(
		id serial primary key,
		val varchar(255) not null
	);
	
grant insert, select on insert_tests to luadbi;
grant usage, select on insert_tests_id_seq to luadbi;

drop table if exists update_tests;
create table update_tests
	(
		id serial primary key,
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
		0,
		true
	),
	(
		'Row 2',
		0,
		true
	),
	(
		'Row 3',
		0,
		false
	),
	(
		'Row 4',
		0,
		true
	);

grant select, update on update_tests to luadbi;


