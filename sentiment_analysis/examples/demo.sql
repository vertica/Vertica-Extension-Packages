-------- Demonstrate the use of your function ----------

CREATE TABLE T (word VARCHAR(200));
COPY T FROM STDIN;
SingleWord
This is an input text file
So possibly is equated with possible 
This is the final data line
Well maybe not!
The quick brown fox jumped over the lazy dog
\.

-- Invoke UDF
SELECT word, removespace(word) FROM T;
DROP TABLE T;
