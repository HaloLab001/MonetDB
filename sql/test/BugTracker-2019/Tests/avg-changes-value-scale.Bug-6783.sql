CREATE TABLE bug6783 (t TIMESTAMP);
INSERT INTO bug6783 values ('1970-01-01 00:02:55.000000');
SELECT t - SYS.STR_TO_TIMESTAMP('0', '%s') FROM bug6783;
SELECT AVG(t - SYS.STR_TO_TIMESTAMP('0', '%s')) FROM bug6783;
