/*

	MySQL test schema.

*/

drop table if exists select_tests;
create table select_tests
	(
		id int not null primary key auto_increment,
	
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

grant select on select_tests to 'luadbi'@'%';


drop table if exists insert_tests;
create table insert_tests
	(
		id int not null primary key auto_increment,
		val varchar(255) not null
	);
	
grant insert, select on insert_tests to 'luadbi'@'%';
