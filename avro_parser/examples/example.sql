------
-- Test for AvroParser user defined parser
------

create table t (f1 varchar,
       f2 date,
       f3 varchar,
       f4 varchar,
       f5 varchar,
       f6 varchar,
       f7 varchar,
       f8 varchar,
       f9 varchar,
       f10 varchar,
       f11 varchar,
       f12 varchar,
       f13 varchar,
       f14 varchar,
       f15 varchar,
       f16 varchar,
       f17 varchar,
       f18 varchar,
       f19 varchar,
       f20 varchar,
       f21 varchar,
       f22 varchar,
       f23 varchar,
       f24 varchar,
       f25 varchar,
       f26 varchar,
       f27 varchar,
       f28 varchar,
       f29 varchar,
       f30 varchar,
       f31 varchar,
       f32 varchar,
       f33 varchar,
       f34 varchar,
       f35 varchar,
       f36 varchar,
       f37 varchar,
       f38 varchar,
       f39 varchar,
       f40 varchar,
       f41 varchar,
       f42 varchar,
       f43 varchar,
       f44 varchar,
       f45 varchar,
       f46 varchar,
       f47 varchar,
       f48 varchar,
       f49 varchar,
       f50 varchar,
       f51 varchar,
       f52 varchar,
       f53 varchar);

\set file `echo "'$TMPDIR/examples/film-film.avro'"`
copy t from :file with parser AvroParser() no commit; 

select * from t;
drop table t;

create table t (station varchar, time int, temp int);

\set file `echo "'$TMPDIR/examples/weather.avro'"`
copy t from :file with parser AvroParser() no commit; 

select * from t;
drop table t;

create table t (name varchar, constit varchar, addr varchar, comprehensive boolean, type varchar);

\set file `echo "'$TMPDIR/examples/medicine-cancer_center.avro'"`
copy t from :file with parser AvroParser() no commit; 

select * from t;
drop table t;
