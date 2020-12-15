CREATE TABLE "experiment" (
	"exp_id"    int PRIMARY KEY,
	"user_id"   int,
	"config_id" int,
	"table_id"  int,
	"result_id" int
);

CREATE TABLE "tapestry_user" (
	"user_id"    int PRIMARY KEY,
	"name"       varchar(25),
	"email"      varchar(50),
	"createdate" date,
	"passwd"     varchar(50)
);

CREATE TABLE "config" (
	"config_id" int PRIMARY KEY,
	"name"      varchar(25),
	"target_id" int,
	"sysinf_id" int
);

CREATE TABLE "tapestry_table" (
	"table_id" int PRIMARY KEY,
	"nrrows"   int,
	"nrcols"   int,
	"seed"     int,
	"fast"     int
);

CREATE TABLE "result" (
	"result_id"   int PRIMARY KEY,
	"type"        varchar(10),
	"description" varchar(256)
);

CREATE TABLE "target" (
	"target_id"  int PRIMARY KEY,
	"name"       varchar(50),
	"permission" varchar(10),
	"comment"    varchar(256)
);

CREATE TABLE "sysinf" (
	"sysinf_id"   int PRIMARY KEY,
	"platform_id" int,
	"cpu_id"      int,
	"memory_id"   int,
	"disk_id"     int
);
 
CREATE TABLE "platform" (
	"platform_id" int PRIMARY KEY,
	"name"        varchar(50),
	"comment"     varchar(256)
);

CREATE TABLE "cpu" (
	"cpu_id" int PRIMARY KEY,
	"type"   varchar(50)
);

CREATE TABLE "memory" (
	"memory_id" int PRIMARY KEY,
	"type"      varchar(50),
	"size"      int
);

CREATE TABLE "disk" (
	"disk_id" int PRIMARY KEY,
	"type"    varchar(50),
	"size"    int
);

CREATE TABLE "query_walk" (
	"walk_id"   int PRIMARY KEY,
	"begin_x"   int,
	"begin_y"   int,
	"nr_runs"   int,
	"nr_steps"  int,
	"step_size" int,
	"end_x"     int,
	"end_y"     int
);

ALTER TABLE "experiment" ADD FOREIGN KEY ("user_id")
	REFERENCES "tapestry_user" ("user_id");
ALTER TABLE "experiment" ADD FOREIGN KEY ("config_id")
	REFERENCES "config" ("config_id");
ALTER TABLE "experiment" ADD FOREIGN KEY ("table_id")
	REFERENCES "tapestry_table" ("table_id");
ALTER TABLE "experiment" ADD FOREIGN KEY ("result_id")
	REFERENCES "result" ("result_id");

ALTER TABLE "config" ADD FOREIGN KEY ("target_id")
	REFERENCES "target" ("target_id");
ALTER TABLE "config" ADD FOREIGN KEY ("sysinf_id")
	REFERENCES "sysinf" ("sysinf_id");

ALTER TABLE "sysinf" ADD FOREIGN KEY ("platform_id")
	REFERENCES "platform" ("platform_id");
ALTER TABLE "sysinf" ADD FOREIGN KEY ("cpu_id")
	REFERENCES "cpu" ("cpu_id");
ALTER TABLE "sysinf" ADD FOREIGN KEY ("memory_id")
	REFERENCES "memory" ("memory_id");
ALTER TABLE "sysinf" ADD FOREIGN KEY ("disk_id")
	REFERENCES "disk" ("disk_id");

select "name", "query", "type", "remark" from describe_table('sys', 'experiment');
select "name", "query", "type", "remark" from describe_table('sys', 'tapestry_user');
select "name", "query", "type", "remark" from describe_table('sys', 'config');
select "name", "query", "type", "remark" from describe_table('sys', 'tapestry_table');
select "name", "query", "type", "remark" from describe_table('sys', 'result');
select "name", "query", "type", "remark" from describe_table('sys', 'target');
select "name", "query", "type", "remark" from describe_table('sys', 'sysinf');
select "name", "query", "type", "remark" from describe_table('sys', 'platform');
select "name", "query", "type", "remark" from describe_table('sys', 'cpu');
select "name", "query", "type", "remark" from describe_table('sys', 'memory');
select "name", "query", "type", "remark" from describe_table('sys', 'disk');
select "name", "query", "type", "remark" from describe_table('sys', 'query_walk');

select * from describe_columns('sys', 'experiment');
select * from describe_columns('sys', 'tapestry_user');
select * from describe_columns('sys', 'config');
select * from describe_columns('sys', 'tapestry_table');
select * from describe_columns('sys', 'result');
select * from describe_columns('sys', 'target');
select * from describe_columns('sys', 'sysinf');
select * from describe_columns('sys', 'platform');
select * from describe_columns('sys', 'cpu');
select * from describe_columns('sys', 'memory');
select * from describe_columns('sys', 'disk');
select * from describe_columns('sys', 'query_walk');

DROP TABLE "query_walk" CASCADE;
DROP TABLE "disk" CASCADE;
DROP TABLE "memory" CASCADE;
DROP TABLE "cpu" CASCADE;
DROP TABLE "platform" CASCADE;
DROP TABLE "sysinf" CASCADE;
DROP TABLE "target" CASCADE;
DROP TABLE "result" CASCADE;
DROP TABLE "tapestry_table" CASCADE;
DROP TABLE "config" CASCADE;
DROP TABLE "tapestry_user" CASCADE;
DROP TABLE "experiment" CASCADE;
