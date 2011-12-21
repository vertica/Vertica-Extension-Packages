
create schema groups;
set search_path = public,groups;

create table groups.T(a varchar(20), b varchar(20), c varchar(20), d varchar(20));

copy T from stdin delimiter ',' direct;
1, 1, 1, 1
1, 1, 1, 2
1, 1, 1, 3
1, 1, 2, 1
1, 1, 2, 2
1, 1, 2, 3
1, 2, 1, 1
1, 2, 1, 2
1, 2, 1, 3
1, 2, 2, 1
1, 2, 2, 2
1, 2, 2, 3
1, 4, 1, 1
1, 4, 1, 2
1, 4, 1, 3
1, 4, 2, 1
1, 4, 2, 2
1, 4, 2, 3
\.

create view groups.rollup_groups1 as select group_generator_3(a, b, c, 95, 4) over() as (a, b, c, id) from groups.T;

select * from rollup_groups1 order by id, a, b, c;

-- ROLLUP(a, b, c)
select a, b, c, id from rollup_groups1 group by a, b, c, id order by id, a, b, c;

-- compared w/ ROLLUP w/ UNION ALL
(select a, b, c, 0 as id from T group by a, b, c, id) -- 000
union all
(select a, b, null, 1 as id from T group by a, b, id) -- 001
union all
(select a, null, null, 3 as id from T group by a, id) -- 011
union all
(select null, null, null, 7 as id from T group by id) -- 111
;

create view groups.rollup_groups2 as select group_generator_4(a, b, c, d, 622, 4) over() as (a, b, c, d, id) from groups.T;

select * from rollup_groups2 order by id, a, b, c, d;

-- sum(d), ROLLUP(a, b, c)
select a, b, c, sum(d), id from rollup_groups2 group by a, b, c, id order by id, a, b, c;

-- compared w/ ROLLUP w/ UNION ALL
(select a, b, c, sum(d) sum_d, 0 as id from T group by a, b, c, id) -- 0000
union all
(select a, b, null, sum(d), 2 as id from T group by a, b, id) -- 0010
union all
(select a, null, null, sum(d), 6 as id from T group by a, id) -- 0110
union all
(select null, null, null, sum(d), 14 as id from T group by id) -- 1110
;


create view groups.cube_groups as select group_generator_3(a, b, c, 342391, 8) over() as (a, b, c, id) from groups.T;

-- CUBE(a, b, c)
select a, b, c, id from cube_groups group by a, b, c, id order by id, a, b, c;

-- compare w/ CUBE w/ UNION ALL
(select a, b, c, 0 as id from T group by a, b, c, id) -- 000
union all
(select a, b, null, 1 as id from T group by a, b, id) -- 001
union all
(select a, null, c, 3 as id from T group by a, c, id) -- 010
union all
(select a, null, null, 3 as id from T group by a, id) -- 011
union all
(select null, b, c, 7 as id from T group by b, c, id) -- 100
union all
(select null, b, null, 7 as id from T group by b, id) -- 101
union all
(select null, null, c, 7 as id from T group by c, id) -- 110
union all
(select null, null, null, 7 as id from T group by id) -- 111
;

-- GROUPING_SETS( (C, A), (C, B) )
create view groups.grouping_sets as select group_generator_3(c, a, b, 10, 2) over() as (c, a, b, id) from groups.T;

select c, a, b, id from grouping_sets group by c, a, b, id order by id, c, a, b;

-- compae w/ GROUPING_SETS w/ UNION ALL
select * from 
((select c, a, null as b, 1 as id from T group by c, a, id) -- 001
union all
(select c, null, b, 2 as id from T group by c, b, id) -- 010
) T
order by id, c, a, b;


-- Do MULTIPLE DISTINCT aggregates
-- This keeps us in the New EE!!!!

select c, count(a), count(b) from
(select c, a, b, id from grouping_sets group by c, a, b, id order by id, c, a, b) T
group by c;

select c, count(distinct a), count(distinct b) from T group by c;



 -- Test w/ different types
create table groups.T2(a float, b varchar(20), c float, d varchar(20));

copy T2 from stdin delimiter ',' direct;
1, 1, 1, 1
1, 1, 1, 2
1, 1, 1, 3
1, 1, 2, 1
1, 1, 2, 2
1, 1, 2, 3
1, 2, 1, 1
1, 2, 1, 2
1, 2, 1, 3
1, 2, 2, 1
1, 2, 2, 2
1, 2, 2, 3
1, 4, 1, 1
1, 4, 1, 2
1, 4, 1, 3
1, 4, 2, 1
1, 4, 2, 2
1, 4, 2, 3
1.1, 4.1, 2.1, 3.1
\.

create view groups.rollup_groups3 as select group_generator_3(a, b, c, 95, 4) over() as (a, b, c, id) from groups.T2;
create view groups.rollup_groups4 as select group_generator_4(a, b, c, d, 4991, 5) over() as (a, b, c, d, id) from groups.T2;

select * from rollup_groups3 order by id, a, b, c;
select * from rollup_groups4 order by id, a, b, c, d;

-- ROLLUP(a, b, c)
select a, b, c, id from rollup_groups3 group by a, b, c, id order by id, a, b, c;

-- ROLLUP w/ UNION ALL
(select a, b, c, 0 as id from T2 group by a, b, c, id)
union all
(select a, b, null::float, 1 as id from T2 group by a, b, id)
union all
(select a, null, null::float, 3 as id from T2 group by a, id)
union all
(select null::float, null, null::float, 7 as id from T2 group by id)
;

-- ROLLUP(a, b, c, d)
select a, b, c, d, id from rollup_groups4 group by a, b, c, d, id order by id, a, b, c, d;

(select a, b, c, d, 0 as id from T2 group by a, b, c, d, id) -- 0000
union all
(select a, b, c, null, 1 as id from T2 group by a, b, c, id) -- 0001
union all
(select a, b, null::float, null, 3 as id from T2 group by a, b, id) -- 0011
union all
(select a, null, null::float, null, 7 as id from T2 group by a, id) -- 0111
union all
(select null::float, null, null::float, null, 15 as id from T2 group by id) --- 1111
;

drop schema groups cascade;
