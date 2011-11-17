CREATE TABLE sentences (sentence varchar(4000));

-- from http://www.eslbee.com/sentences.htm
COPY sentences FROM STDIN;
I tried to speak Spanish, and my friend tried to speak English.  
Alejandro played football, so Maria went shopping.  
Alejandro played football, for Maria went shopping.
When he handed in his homework, he forgot to give the teacher the last page.  
The teacher returned the homework after she noticed the error. 
The students are studying because they have a test tomorrow.
After they finished studying, Juan and Maria went to the movies. 
Juan and Maria went to the movies after they finished studying.
The woman who(m) my mom talked to sells cosmetics.
The book that Jonathan read is on the shelf.
The house which Abraham  Lincoln was born in is still standing.
The town where I grew up is in the United States.
\.

SELECT sentence, WORDCOUNT(sentence) AS word_count FROM sentences;

DROP TABLE sentences CASCADE;

