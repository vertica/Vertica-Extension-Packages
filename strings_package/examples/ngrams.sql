---------------
-- Example of using ngram functions
---------------

CREATE TABLE sentences (sentence varchar(300));
COPY sentences FROM stdin;
SingleWord
This is an input text file
So possibly is equated with possible 
This is the final data line
Well maybe not!
The quick brown fox jumped over the lazy dog
\.

\echo ***************** 2Grams: ***************** 
select sentence, 
	   twograms(sentence) over(partition by sentence) 
from sentences;

\echo ***************** 3Grams: ***************** 
select sentence, 
	   threegrams(sentence) over(partition by sentence) 
from sentences;

\echo ***************** 4Grams: ***************** 
select sentence, 
	   fourgrams(sentence) over(partition by sentence) 
from sentences;

\echo ***************** 5Grams: ***************** 
select sentence, 
	   fivegrams(sentence) over(partition by sentence) 
from sentences;

drop table sentences;
