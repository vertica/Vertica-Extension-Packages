select '''' || fname || ''' ON ' || node_name from (select rank() over (order by node_id) as r, node_name from nodes) nodes, (select rank() over (order by fname) as r, fname from files) files where nodes.r = (files.r % 3) + 1;

select name, b.command, b.text from (select name, shell_execute('stat ~/5.0_CATANIA/vertica/SQLTest/' || fname) over (partition by name) from (select local_node_name() as name,fname from files) a) b;
