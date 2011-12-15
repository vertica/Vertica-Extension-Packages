-- Installation script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/Shell.so\'';

CREATE LIBRARY ShellLib as :libfile;
CREATE FUNCTION hostname as language 'C++' name 'HostnameFactory' library ShellLib;
CREATE TRANSFORM FUNCTION shell_execute as language 'C++' name 'ShellFactory' library ShellLib;
CREATE TRANSFORM FUNCTION int_sequence as language 'C++' name 'IntSequenceFactory' library ShellLib;

-- code to generate a segmented table with exactly 1 row per node
CREATE TABLE allnodes (segval int) segmented by hash(segval) all nodes ksafe;
-- create a temp table to store at least one row per node
CREATE LOCAL TEMP TABLE stageallnodes (a int) segmented by hash(a) all nodes;
-- load table with enough rows so that hash function is highly likely to get at least one on each node
INSERT INTO stageallnodes select int_sequence(1,cnt*10) over () from (select count(*) as cnt from nodes) n;
-- use last_value partitioned by node and group by to get exactly one integer which should hash onto each node
INSERT INTO allnodes select lv from (select name,last_value(a) over (partition by name order by a rows between unbounded preceding and unbounded following) as lv from (select local_node_name() as name,a from stageallnodes) a) b group by lv;
COMMIT;
DROP TABLE stageallnodes;

-- helper 
CREATE VIEW onallnodes as SELECT local_node_name() as node_name FROM allnodes;
