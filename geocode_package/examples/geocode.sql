-------- Geocode ----------

CREATE TABLE T (streetaddress VARCHAR(200), 
		city VARCHAR(128), 
		state VARCHAR(128), 
		zip VARCHAR(128));
COPY T FROM STDIN ENCLOSED BY '"';
8 Federal St|Billerica|MA|01821
3000 Hanover Street|Palo Alto|CA|94304
""|Chicago|IL|""
\.

-- Invoke UDF
SELECT streetaddress,city,state,zip,geocode(streetaddress,city,state,zip) FROM T;
DROP TABLE T;
