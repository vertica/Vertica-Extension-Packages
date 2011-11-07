CREATE TABLE T (url varchar(30), description varchar(2000));
INSERT INTO T VALUES ('www.amazon.com','Online retail merchant and provider of cloud services');
INSERT INTO T VALUES ('www.hp.com','Leading provider of computer hardware and imaging solutions');
INSERT INTO T VALUES ('www.vertica.com','World''s fastest analytic database');
COMMIT;
-- Invoke the UDT
SELECT url, StringTokenizer(description) OVER (partition by url) FROM T;
DROP TABLE T;
