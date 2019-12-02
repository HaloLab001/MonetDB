CREATE TABLE tab0(col0 INTEGER, col1 INTEGER, col2 INTEGER);
INSERT INTO tab0 VALUES(83,0,38), (26,0,79), (43,81,24);
SELECT DISTINCT col2 FROM tab0 GROUP BY col2, col1 HAVING NOT NULL NOT IN ( AVG ( ALL + col1 ) );
SELECT 1 FROM tab0 HAVING tab0.col1 IN ( 1 ); --error
SELECT 1 FROM tab0 HAVING col1 IN ( 1 ); --error
SELECT 1 FROM tab0 HAVING NULL IN ( tab0.col2 ); --error
SELECT DISTINCT tab0.col1 AS col2 FROM tab0 GROUP BY tab0.col1 HAVING NULL NOT IN ( col2 ); --error
SELECT DISTINCT * FROM tab0 AS cor0 GROUP BY cor0.col1, cor0.col2, cor0.col0;
SELECT CAST(SUM(col0) AS BIGINT) FROM tab0 WHERE + + col0 BETWEEN NULL AND + col2;
SELECT CAST(SUM(col0) AS BIGINT) FROM tab0 WHERE + + col0 NOT BETWEEN NULL AND + col2;
SELECT DISTINCT COUNT(*) FROM tab0 WHERE NOT col2 NOT BETWEEN ( 35 ) AND ( NULL );
SELECT CAST(- COUNT(*) * - - 61 + + + ( + COUNT(*) ) AS BIGINT) FROM tab0 WHERE NOT col0 + + 10 BETWEEN NULL AND NULL;

SELECT ALL * FROM tab0 AS cor0 WHERE col2 BETWEEN NULL AND NULL; --empty
SELECT COUNT ( * ) FROM tab0 WHERE NOT col1 NOT BETWEEN NULL AND NULL; --0
SELECT - 78 * + MAX ( DISTINCT col2 ) + - 52 AS col1 FROM tab0 AS cor0 WHERE NOT - col0 + col2 NOT BETWEEN ( NULL ) AND NULL; --NULL
DROP TABLE tab0;
