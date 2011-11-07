-- do setup for this package (i.e., install the package and create the functions)
select version();

CREATE TABLE T (sentence VARCHAR(200));

COPY T FROM STDIN;
SingleWord
This is an input text file
So possibly is equated with possible 
This is the final data line
Well maybe not!
The quick brown fox jumped over the lazy dog
\.


