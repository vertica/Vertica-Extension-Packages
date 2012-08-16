create table t (re float, im float);

\set file `echo "'$TMPDIR/examples/test5000.avro'"`
copy t from :file with parser AvroParser() no commit; 

select * from t;
drop table t;
