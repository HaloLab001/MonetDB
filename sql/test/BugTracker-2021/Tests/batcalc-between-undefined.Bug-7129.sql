start transaction;

create table t_qh ( c_f INTEGER , c_y2 INTEGER , primary key(c_f), unique(c_f) );

WITH cte_1 AS (select count( cast(87.53 as INTEGER)) as c0, avg( cast(abs( cast(50.40 as INTEGER)) as INTEGER)) as c1, subq_0.c0 as c2 from
(select distinct ref_5.c_f as c0, 75 as c1, ref_5.c_f as c2 from t_qh as ref_5 where ref_5.c_f is not NULL) as subq_0 group by subq_0.c0)
select distinct cast(sum( cast((case when (ref_23.c0 > ref_23.c0) and (ref_23.c0 < ref_23.c1) then ref_23.c1 else ref_23.c1 end & ref_23.c1) as INTEGER)) as bigint)
as c3, ref_23.c0 as c4 from cte_1 as ref_23 group by ref_23.c0;

rollback;
