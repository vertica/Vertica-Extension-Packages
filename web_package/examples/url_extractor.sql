CREATE TABLE urls (url varchar(1000));

COPY urls from stdin;
http://www.google.com/search?q=lm311+video+sync+schematic&hl=id&lr=&ie=UTF-8&start=10&sa=N
http://www.yahoo.com/search?q=lm311+videooooo+sy%3Fnc+schematic&hl=id&lr=&ie=UTF-8&start=10&sa=Nhgvhm
\.


select UriExtractor(url) OVER () from urls;

DROP TABLE urls CASCADE;

