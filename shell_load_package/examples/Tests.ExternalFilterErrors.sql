\! seq 1 10 > $TMPDIR/sample.dat
\set datafile '\''`echo $TMPDIR`'/sample.dat\''
create table t (i integer);

copy t from :datafile with filter ExternalFilter(cmd='cat') no commit;

copy t with source ExternalSource(cmd='exit 7') no commit;

copy t with source ExternalSource(cmd='echo "message on stderr" >&2') no commit;

copy t with source ExternalSource(cmd='echo "message on stderr and exit" >&2; exit 8') no commit;

drop table t;
\! rm $TMPDIR/sample.dat
