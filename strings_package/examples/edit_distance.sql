create table words (word varchar(20));

copy words from stdin;
This
this
Thus
The
There
To
Three
\.

select 
	   word, 
	   EditDistance(word, 'This') this_distance,
	   EditDistance(word, 'Tau')  tau_distance
from words;

drop table words;
