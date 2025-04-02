/*

	DuckDB test schema.

*/

create sequence select_ids start 1;
create table select_tests
	(
		id int primary key default nextval( 'select_ids' ),

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


create sequence insert_ids start 1;
create table insert_tests
	(
		id int primary key default nextval( 'insert_ids' ),
		val varchar(255) null
	);


create sequence update_ids start 1;
create table update_tests
	(
		id int primary key default nextval( 'update_ids' ),
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
