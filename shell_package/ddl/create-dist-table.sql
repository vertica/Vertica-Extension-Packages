-- code to generate a segmented table with exactly 1 row per node
CREATE TABLE :tablename (segval int) segmented by hash(segval) :nodes ksafe;
-- create a temp table to store at least one row per node
CREATE LOCAL TEMP TABLE stage:tablename (a int) segmented by hash(a) :nodes;
-- load table with enough rows so that hash function is highly likely to get at least one on each node
INSERT INTO stage:tablename select int_sequence(1,cnt*10) over () from (select count(*) as cnt from nodes) n;
-- use last_value partitioned by node and group by to get exactly one integer which should hash onto each node
INSERT /*+direct*/ INTO :tablename select lv from (select name,last_value(a) over (partition by name order by a rows between unbounded preceding and unbounded following) as lv from (select local_node_name() as name,a from stage:tablename) a) b group by lv;
COMMIT;
DROP TABLE stage:tablename;

-- helper 
CREATE VIEW on:tablename as SELECT local_node_name() as node_name, segval FROM :tablename;
