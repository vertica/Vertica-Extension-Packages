
create table t (i integer);

copy t with source ExternalSource(cmd='seq 1 10') no commit;
select * from t;
rollback;

drop table t;
