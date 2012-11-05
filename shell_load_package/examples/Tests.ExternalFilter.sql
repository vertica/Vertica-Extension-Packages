\! seq 1 10 > $TMPDIR/sample.dat
\set datafile '\''`echo $TMPDIR`'/sample.dat\''
create table t (i integer);

copy t from :datafile with filter ExternalFilter(cmd='cat') no commit;
select * from t order by i asc;
rollback;

copy t from :datafile with
     filter ExternalFilter(cmd='cat')
     filter ExternalFilter(cmd='cat')
     filter ExternalFilter(cmd=E'awk \'{ print $1 }\'')
     no commit;
select * from t order by i asc;
rollback;

copy t from :datafile with
     filter ExternalFilter(cmd='cat')
     filter ExternalFilter(cmd='tac')
     filter ExternalFilter(cmd=E'awk \'{ print $1 }\'')
     no commit;
select * from t order by i desc;
rollback;

copy t from :datafile with filter ExternalFilter(cmd='tac') no commit;
select * from t order by i desc;
rollback;

drop table t;
create table t (i integer, j integer);

copy t from :datafile with
     filter ExternalFilter(cmd='cat -n')
     filter ExternalFilter(cmd=E'awk \'{ print $1 "," $2 }\'')
     delimiter ',' no commit;
select * from t order by i asc;
rollback;

copy t from :datafile with
     filter ExternalFilter(cmd='tac')
     filter ExternalFilter(cmd='cat -n')
     filter ExternalFilter(cmd=E'awk \'{ print $1 "," $2 }\'')
     delimiter ',' no commit;
select * from t order by i asc;
rollback;

copy t from :datafile with
     filter ExternalFilter(cmd=E'tac | cat -n | awk \'{ print $1 "," $2 }\'')
     delimiter ',' no commit;
select * from t order by i asc;
rollback;

drop table t;
\! rm $TMPDIR/sample.dat
