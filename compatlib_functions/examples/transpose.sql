
create table people (id int, name varchar(20), gender varchar(1));
insert into people values (1, 'Patrick', 'M');
insert into people values (2, 'Jim', 'M');
insert into people values (3, 'Sandy', 'F');
insert into people values (4, 'Brian', 'M');
insert into people values (5, 'Linda', 'F');
commit;

select transpose(gender, name, ', ') over (partition by gender) from people;

drop table people cascade;