
-- Tokenize on space
SELECT StringTokenizer(sentence) OVER () FROM T;

-- Tokenize on the letter 'i'
SELECT StringTokenizerDelim(sentence, 'i') OVER () FROM T;

-- How far away is each word in the sentences from 'is'?
SELECT word, EditDistance(word, 'is') FROM 
(SELECT StringTokenizer(sentence) OVER () as word FROM T) as SQ;
