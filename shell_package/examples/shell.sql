-- the following are a number of crazy things to do with the shell command

-- want some vertica-independent machine statistics?
select node_name,shell_execute('vmstat 1 5') over (partition by node_name) from onallnodes order by node_name;

-- look at vertica log?
select node_name,shell_execute('tail -100 vertica.log') over (partition by node_name) from onallnodes order by node_name;

