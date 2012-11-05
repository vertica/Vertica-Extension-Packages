
create table t (i integer);
create table t2 (v varchar(1000));

copy t with source ExternalSource(cmd='seq 1 10') no commit;
select * from t order by i asc;
rollback;

copy t2 with source ExternalSource(cmd='echo "x${CURRENT_NODE_NAME}x"') no commit;
select count(*) from t2 where v = 'xx';
select count(*) from t2 where v <> 'xx';
rollback;

drop table t;
drop table t2;
