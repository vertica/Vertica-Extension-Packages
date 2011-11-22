CREATE TABLE words (word varchar(300));
COPY words FROM stdin;
light
lights
possibly
possible
possibility
lighthouse
lighthouses
\.


select word, stemmer(word) as stemmed from words;
drop table words;


