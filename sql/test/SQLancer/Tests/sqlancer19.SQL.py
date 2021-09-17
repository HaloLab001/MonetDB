import os

from MonetDBtesting.sqltest import SQLTestCase

port = os.environ['MAPIPORT']
db = os.environ['TSTDB']

with SQLTestCase() as cli:
    cli.connect(username="monetdb", password="monetdb")
    cli.execute("""
    START TRANSACTION;
    CREATE TABLE "t0" ("c0" INTERVAL SECOND NOT NULL, "c1" JSON);
    INSERT INTO "t0" VALUES (INTERVAL '9' SECOND, '""');

    CREATE TABLE "t1" ("c0" BINARY LARGE OBJECT,"c1" BIGINT);
    INSERT INTO "t1" VALUES (NULL, 1),(NULL, 6),(NULL, 0),(BINARY LARGE OBJECT '50', NULL),(BINARY LARGE OBJECT 'ACBC2EDEF0', NULL),
    (BINARY LARGE OBJECT '65', NULL),(BINARY LARGE OBJECT 'EF43C0', NULL),(BINARY LARGE OBJECT '90', NULL),(BINARY LARGE OBJECT '', NULL);

    CREATE TABLE "t3" ("c0" BIGINT,"c1" INTERVAL MONTH);
    INSERT INTO "t3" VALUES (1, INTERVAL '9' MONTH),(5, INTERVAL '6' MONTH),(5, NULL),(7, NULL),(2, INTERVAL '1' MONTH),(2, INTERVAL '1' MONTH);
    COMMIT;

    START TRANSACTION;
    CREATE REMOTE TABLE "rt1" ("c0" BINARY LARGE OBJECT,"c1" BIGINT) ON 'mapi:monetdb://localhost:%s/%s/sys/t1';
    CREATE REMOTE TABLE "rt3" ("c0" BIGINT,"c1" INTERVAL MONTH) ON 'mapi:monetdb://localhost:%s/%s/sys/t3';
    COMMIT;""" % (port, db, port, db)).assertSucceeded()

    cli.execute('SELECT json."integer"(JSON \'1\') FROM rt3;').assertSucceeded().assertDataResultMatch([(1,),(1,),(1,),(1,),(1,),(1,)])

    cli.execute('MERGE INTO t0 USING (SELECT 1 FROM rt1) AS mergejoined(c0) ON TRUE WHEN NOT MATCHED THEN INSERT (c0) VALUES (INTERVAL \'5\' SECOND);') \
        .assertSucceeded().assertRowCount(0)

    cli.execute("""
    START TRANSACTION;
    DROP TABLE rt1;
    DROP TABLE rt3;
    DROP TABLE t0;
    DROP TABLE t1;
    DROP TABLE t3;
    COMMIT;""").assertSucceeded()
