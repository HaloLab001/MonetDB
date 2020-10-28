CREATE TABLE t0(tc0 INTERVAL MONTH DEFAULT (INTERVAL '1997904243' MONTH), tc1 TIME UNIQUE);
INSERT INTO t0(tc0) VALUES(INTERVAL '444375026' MONTH);
DELETE FROM t0 WHERE TRUE;
ALTER TABLE t0 ALTER tc0 SET NOT NULL;
INSERT INTO t0(tc0) VALUES(INTERVAL '-625288924' MONTH);
UPDATE t0 SET tc0 = (t0.tc0) WHERE TRUE;
DROP TABLE t0;

CREATE TABLE t2(tc0 TINYINT);
ALTER TABLE t2 ADD PRIMARY KEY(tc0);
INSERT INTO t2(tc0) VALUES(44), (126), (117);
ALTER TABLE t2 ADD FOREIGN KEY (tc0) REFERENCES t2(tc0) MATCH FULL;
DROP TABLE t2;

START TRANSACTION;
CREATE TABLE "t2" ("tc0" BIGINT NOT NULL,CONSTRAINT "t2_tc0_pkey" PRIMARY KEY ("tc0"),CONSTRAINT "t2_tc0_unique" UNIQUE ("tc0"), CONSTRAINT "t2_tc0_fkey" FOREIGN KEY ("tc0") REFERENCES "t2" ("tc0"));
CREATE TABLE "t0" ("tc0" TINYINT NOT NULL,"tc2" TINYINT NOT NULL,CONSTRAINT "t0_tc2_tc0_unique" UNIQUE ("tc2", "tc0"),CONSTRAINT "t0_tc2_fkey" FOREIGN KEY ("tc2") REFERENCES "t2" ("tc0"));

update t0 set tc2 = 119, tc0 = cast(t0.tc0 as bigint);
update t0 set tc2 = 119, tc0 = (least(+ (cast(least(0, t0.tc0) as bigint)), sign(scale_down(100, 1)))) where true;
ROLLBACK;
