-- get a list of nodes
select group_concat_delim(node_name, '|') over () from nodes;

-- nodes with storage for a projection
select schema_name,projection_name,group_concat_delim(node_name, '|') over (partition by schema_name,projection_name) from (select distinct node_name,schema_name,projection_name from storage_containers) sc order by schema_name, projection_name;
