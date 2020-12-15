-- this should yield in nothing (empty database)
with describe_all_objects as (
 select s.name as sname,
     t.name,
     s.name || '.' || t.name as fullname,
     cast(case t.type
     when 1 then 2
     else 1
     end as smallint) as ntype,
     (case when t.system then 'SYSTEM ' else '' end) || tt.table_type_name as type,
     t.system
   from sys._tables t
   left outer join sys.schemas s on t.schema_id = s.id
   left outer join sys.table_types tt on t.type = tt.table_type_id )
select type, fullname from describe_all_objects where (ntype & 3) > 0 and not system and (sname is null or sname = current_schema) order by fullname, type;
-- create a table
create table bug2861 (id int);
-- we should see it
with describe_all_objects as (
 select s.name as sname,
     t.name,
     s.name || '.' || t.name as fullname,
     cast(case t.type
     when 1 then 2
     else 1
     end as smallint) as ntype,
     (case when t.system then 'SYSTEM ' else '' end) || tt.table_type_name as type,
     t.system
   from sys._tables t
   left outer join sys.schemas s on t.schema_id = s.id
   left outer join sys.table_types tt on t.type = tt.table_type_id )
select type, fullname from describe_all_objects where (ntype & 3) > 0 and not system and (sname is null or sname = current_schema) order by fullname, type;
drop table bug2861;
-- and it should be gone again
with describe_all_objects as (
 select s.name as sname,
     t.name,
     s.name || '.' || t.name as fullname,
     cast(case t.type
     when 1 then 2
     else 1
     end as smallint) as ntype,
     (case when t.system then 'SYSTEM ' else '' end) || tt.table_type_name as type,
     t.system
   from sys._tables t
   left outer join sys.schemas s on t.schema_id = s.id
   left outer join sys.table_types tt on t.type = tt.table_type_id )
select type, fullname from describe_all_objects where (ntype & 3) > 0 and not system and (sname is null or sname = current_schema) order by fullname, type;
