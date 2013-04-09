-- Try some stuff out
-- Requires that you have a local MySQL instance with no root password,
-- and ODBC properly installed and configured to connect to it under the "MySQL" DSN.
\! echo "CREATE DATABASE testdb;" | mysql -u root

-- Populate a table in MySQL
-- Create the timezone-based entries as varchar's; MySQL doesn't do timezones at all AFAICT...
-- MySQL Timestamps automagically rewrite 'null' to CURRENT_TIMESTAMP() unless the column in question explicitly allows null values.  (Older MySQL's have other nonstandard behavior re: timestamps.)
\! echo "CREATE TABLE test_mysql (i integer, b boolean, f float, v varchar(32), c char(32), bn binary(32), vb varbinary(32), d date, t time, ts timestamp null, tz varchar(80), tsz varchar(80), n numeric(10,4));" | mysql -u root testdb
\! (echo "INSERT INTO test_mysql VALUES (null, null, null, null, null, null, null, null, null, null, null, null, null, null, null);"; for i in `seq 1 9`; do echo "INSERT INTO test_mysql VALUES ($i, 1, $i.5, 'test $i', 'test $i', 'test $i', 'test $i', 'test $i', 'test $i', '2000/1/$i', '4:0$i', '2038-01-0$i 03:14:07', '1:2$i:00', 'June 1, 2000 03:2$i EDT', '123456.7890');"; done) | mysql -u root testdb

-- Create the corresponding table in Vertica
CREATE TABLE test_vertica (i integer, b boolean, f float, v varchar(32), c char(32), bn binary(32), vb varbinary(32), d date, t time, ts timestamp, tz timetz, tsz timestamptz, n numeric(18,4));

-- Copy from MySQL into Vertica
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='SELECT * FROM test_mysql;');

-- Verify that Vertica and MySQL have the same contents
SELECT i,b,f,v,trim(c::varchar) as c,bn::binary(8) as bn,vb,d,t,ts,tz,tsz,n FROM test_vertica ORDER BY i,b,f,v;
\! echo "SELECT i,b,f,v,trim(c) as c,cast(bn as binary(8)) as bn,vb,d,t,ts,tz,tsz,n FROM test_mysql ORDER BY i,b,f,v;" | mysql -u root testdb

-- Now try copying from ourselves into ourselves; see what happens
CREATE TABLE test_vertica_no_bin AS SELECT i,b,f,v,c,d,t,ts,tz,tsz,n FROM test_vertica;
COPY test_vertica_no_bin WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=VerticaDSN', query='SELECT * FROM test_vertica_no_bin;');
SELECT i,b,f,v,trim(c::varchar) as c,d,t,ts,tz,tsz,n FROM test_vertica_no_bin ORDER BY i,b,f,v;
DROP TABLE test_vertica_no_bin;

-- Try some invalid commands; make sure they error out correctly
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=InvalidDSN', query='SELECT * FROM test_mysql;');
COPY test_vertica WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=Default', query='hi!');

-- Clean up after ourselves
DROP TABLE test_vertica;
\! echo "DROP DATABASE testdb;" | mysql -u root

