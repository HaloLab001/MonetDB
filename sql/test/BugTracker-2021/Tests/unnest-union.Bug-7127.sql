start transaction;

create table t_qh ( c_f INTEGER , c_y2 INTEGER , c_i768 INTEGER , c_tqx TEXT , c_mknkhml TEXT , primary key(c_f, c_y2), unique(c_y2) );
insert into t_qh values (1,1,1,'a','a'), (2,2,2,'b','b'), (3,3,3,'c','c');

create table t_ckfystsc ( c_kvhq5p INTEGER , c_aifpl INTEGER , c_jf6 TEXT , c_f31ix TEXT NOT NULL, c_lk0zqfa INTEGER , c_qotzuxn INTEGER , c_w_z TEXT , primary key(c_lk0zqfa), unique(c_kvhq5p) );
insert into t_ckfystsc values (1,1,'a','a',1,1,'a'), (2,2,'b','b',2,2,'b'), (3,3,'c','c',3,3,'c');

create table t_irwrntam7 as select ref_0.c_i768 as c0, ref_0.c_i768 as c1, ref_0.c_f as c2, ref_0.c_i768 as c3 from t_qh as ref_0 where 
(ref_0.c_f in ( select ref_1.c_lk0zqfa as c0 from t_ckfystsc as ref_1)) and (ref_0.c_y2 >= ( select ref_0.c_y2 as c0 from t_qh as ref_7 union select ref_0.c_f as c0 from t_qh as ref_9));

select ref_0.c_i768 as c0, ref_0.c_i768 as c1, ref_0.c_f as c2, ref_0.c_i768 as c3 from t_qh as ref_0 where 
(ref_0.c_f in ( select ref_1.c_lk0zqfa as c0 from t_ckfystsc as ref_1)) and (ref_0.c_y2 >= ( select ref_0.c_y2 as c0 from t_qh as ref_7 union select ref_0.c_f as c0 from t_qh as ref_9));

select 1 from t_qh where c_f in (select 2 from t_ckfystsc) and c_y2 = (select c_y2 union select c_f);

rollback;
