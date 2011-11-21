-- See README.txt for usage information. Here are a few examples:

-- Simply encrypy/decrypt a string
SELECT AESDecrypt(AESEncrypt('Test String','passphrase'),'passphrase');

-- Create some toy data. No, that's not my real ssn. :)
CREATE TABLE t1 (firstname VARCHAR, lastname VARCHAR, ssn VARCHAR);
INSERT INTO t1 VALUES ('Aaron','Botsis','532-35-3246');
INSERT INTO t1 VALUES ('Bugs','Bunny','124-63-2444');
INSERT INTO t1 VALUES ('Elmer','Fudd','566-24-6352');
INSERT INTO t1 VALUES ('Wile','Coyote','422-64-2422');
INSERT INTO t1 VALUES ('Jim','Coyote','422-64-2422');

-- Update the ssn column with encrypted SSNs
UPDATE t1 SET ssn=AESEncrypt(ssn,'passphrase');
SELECT * from t1;
SELECT firstname,lastname,AESDecrypt(ssn,'passphrase') from t1;
UPDATE t1 SET ssn=AESDecrypt(ssn,'passphrase');

-- You'll notice Jim and Wile have the same SSN. You can do
-- something like this for a compound key:
UPDATE t1 SET ssn=AESEncrypt(ssn,CONCAT('passphrase',firstname));
SELECT * FROM t1 WHERE lastname='Coyote';

DROP TABLE T1;

