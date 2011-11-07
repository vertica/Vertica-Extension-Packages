CREATE TABLE T (word varchar(100), delim varchar);

COPY T FROM STDIN DELIMITER ',';
SingleWord,dd
Break On Spaces, 
Break:On:Colons,:
\.



COMMIT;
-- Invoke the UDT
SELECT StringTokenizerDelim(word,delim) OVER () FROM T;
DROP TABLE T;
