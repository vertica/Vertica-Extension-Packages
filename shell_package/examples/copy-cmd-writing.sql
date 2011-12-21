-- script to FTP files from an ftp server to temp locations across the
-- cluster and execute a multi-node copy statement to load them.

-- usage:
-- vsql -v tablename=\'table\' -v ftpurl=\'ftp.vertica.com\' -v tmpdir=\'/data/tmp\' -t -f examples/copy-cmd-writing.sql | vsql

-- arguments

--\set tablename '\'foo\''
--\set copyopts '\'DIRECT\''
--\set ftpurl '\'ftp://ftp.vertica.com/shellext/\''
--\set tmpdir '\'/tmp/wgettemp/\''

\o /dev/null
create table files (fname varchar(1000)) segmented by hash(fname) all nodes;
insert into files select text as fname from (select shell_execute('files','wget -q -O - '||:ftpurl ||' | grep File | cut -d ''"'' -f 2') over ()) a;
select * from files;

select shell_execute(local_node_name(), 'wget -nc -q -P '||:tmpdir||local_node_name()||' '||fname) over (partition by fname) from files order by fname;

create table staged as select :tmpdir || text as file, id as node_name from (select shell_execute(node_name, 'ls '||:tmpdir||node_name) over (partition by segval) from onallnodes order by id) a;

\o
-- requires string_extensions
select 'copy ' || :tablename || ' from ' || srcs || ' ' || :copyopts || ';' from (select group_concat(file || ' on ' || node_name) over () as srcs from staged) files;

\o /dev/null
drop table files;
drop table staged;
\o
