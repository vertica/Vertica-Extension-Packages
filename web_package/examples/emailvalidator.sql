
/***Example  ***/
CREATE TABLE emails(email_address varchar(100), subject varchar(500));
INSERT INTO emails VALUES ('abc@xyz.com','Lorem ipsum dolor sit amet, consectetur');
INSERT INTO emails VALUES ('www.hp.com','Quisque et feugiat turpis. Integer viverra');
INSERT INTO emails VALUES ('3e1*@VerticaCorp.com','Quisque et mi lorem, ut porttitor lorem');
INSERT INTO emails VALUES ('me@VerticaCorp.com','ut porttitor lorem');
COMMIT;

-- Invoke the UDT
SELECT email_address, emailvalidate(email_address) OVER (partition by email_address) FROM emails;
DROP TABLE emails;


