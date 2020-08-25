-- Try some stuff out
-- Requires that you have a local MySQL instance with no root password,
-- and ODBC properly installed and configured to connect to it under the "MySQL" DSN.
\! echo "CREATE DATABASE testdb;" | mysql -u root

-- Set output to a fixed timezone regardless of where this is being tested
set time zone to 'EST';

-- Populate a table in MySQL
-- Create the timezone-based entries as varchar's; MySQL doesn't do timezones at all AFAICT...
-- MySQL Timestamps automagically rewrite 'null' to CURRENT_TIMESTAMP() unless the column in question explicitly allows null values.  (Older MySQL's have other nonstandard behavior re: timestamps.)
\! echo "CREATE TABLE test_mysql (i integer, b boolean, f float, v varchar(32), c char(32), lv varchar(9999), bn binary(32), vb varbinary(32), lvb varbinary(9999), d date, t time, ts timestamp null, tz varchar(80), tsz varchar(80), n numeric(20,4));" | mysql -u root testdb
\! (echo "INSERT INTO test_mysql VALUES (null, null, null, null, null, null, null, null, null, null, null, null, null, null, null);"; for i in `seq 1 9`; do echo "INSERT INTO test_mysql VALUES ($i, 1, $i.5, 'test $i', 'test $i', 'test $i', 'test $i', 'test $i', 'test $i', '2000/1/$i', '4:0$i', '2038-01-0$i 03:14:07 EST', '1:2$i:00', 'June 1, 2000 03:2$i EST', '123456.7890');"; done) | mysql -u root testdb

-- Create the corresponding table in Vertica
CREATE TABLE test_vertica (i integer, b boolean, f float, v varchar(32), c char(32), lv varchar(9999), bn binary(32), vb varbinary(32), lvb varbinary(999), d date, t time, ts timestamp, tz timetz, tsz timestamptz, n numeric(18,4));

-- Copy from MySQL into Vertica
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='SELECT * FROM testdb.test_mysql;');

-- Verify that Vertica and MySQL have the same contents
SELECT i,b,f,v,trim(c::varchar) as c,lv,bn::binary(8) as bn,vb,lvb,d,t,ts,tz,tsz,n FROM test_vertica ORDER BY i,b,f,v;
\! echo "SELECT i,b,f,v,trim(c) as c,lv,cast(bn as binary(8)) as bn,vb,lvb,d,t,ts,tz,tsz,n FROM test_mysql ORDER BY i,b,f,v;" | mysql -u root testdb

-- Now try copying from ourselves into ourselves; see what happens
CREATE TABLE test_vertica_no_bin AS SELECT i,b,f,v,c,lv,d,t,ts,tz,tsz,n FROM test_vertica;
-- run this in a separate session so it uses the local time zone
COPY test_vertica_no_bin WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=VerticaDSN;ConnSettings=set+session+timezone+to+''EST''', query='SELECT * FROM test_vertica_no_bin;');

SELECT i,b,f,v,trim(c::varchar) as c,lv,d,t,ts,tz,tsz,n FROM test_vertica_no_bin ORDER BY i,b,f,v;
DROP TABLE test_vertica_no_bin;

-- try some failures
\! echo "INSERT INTO test_mysql VALUES (11, null, null, null, null, null, null, null, null, null, null, null, null, null, '99999999999999999999');"  | mysql -u root testdb
\! echo "INSERT INTO test_mysql VALUES (12, null, null, null, null, null, null, null, null, null, null, null, null, 'June 1, 2000 03:2X EST', null);"  | mysql -u root testdb
\! echo "INSERT INTO test_mysql VALUES (13, null, null, null, null, null, null, null, null, null, null, null, '1:2:A EST', null, null);"  | mysql -u root testdb
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='SELECT * FROM testdb.test_mysql where i=11;');
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='SELECT * FROM testdb.test_mysql where i=12;');
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='SELECT * FROM testdb.test_mysql where i=13;');
select * from test_vertica where i>10;

-- Try some invalid commands; make sure they error out correctly
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=InvalidDSN', query='SELECT * FROM testdb.test_mysql;');
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='hi!');

-- Clean up after ourselves
DROP TABLE test_vertica;
\! echo "DROP DATABASE testdb;" | mysql -u root

