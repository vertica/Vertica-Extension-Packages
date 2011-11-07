create table company (id int, supervisor_id int, name varchar(20));
  insert into company values (1, null, 'Patrick');
  insert into company values (2, 1, 'Jim');
  insert into company values (3, 1, 'Sandy');
  insert into company values (4, 3, 'Brian');
  insert into company values (4, 3, 'Otto');
commit;


select connect_by_path(supervisor_id, id, name, ' >> ') over () from company;

drop table company;
