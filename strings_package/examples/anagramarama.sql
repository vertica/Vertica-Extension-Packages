drop table if exists words cascade;
create table words(word varchar(8));
copy words from '/home/chuck/Downloads/anagramarama-0.4/i18n/en_GB/wordlist.txt';

-- How many words total?  This is the number of puzzles there are.
select count(*) from words;
-- How many words have 7 letters?  This is the number of puzzles there are.
-- There are 13699 ways to arrange the letters in a 7-letter word
--  (not taking duplicates into account, but including shorter words).
select count(*) from words where length(word) = 7;

-- Load the UDX
\set libfile '\''`echo $TMPDIR`'/AnagramLib.so\'';
create library AnagramLib as :libfile;
create transform function gen_anagram as language 'C++'
  name 'AnagramFactory'
  library AnagramLib not fenced;

-- A simple test - the first puzzle
select word from words where length(word) = 7 order by word limit 1;

-- What are the real words that are in this puzzle?
select anagram from
(
  select word, gen_anagram(word) over (partition by word)
  from 
  (
    select 'abalone' as word
--    select 'vicious' as word
--    select 'gimmick' as word
--    select 'alfalfa' as word
--    select 'plaster' as word
--    select 'stapler' as word
--    select 'ococomr' as word
  ) puzzle
) sq
where anagram in (select word from words)
group by anagram
order by length(anagram), anagram;

-- What puzzles have the most words?
\timing
select word, count(distinct anagram) wcount from
(
  select word, gen_anagram(word) over (partition by word)
  from 
  (
    select word from words where length(word) = 7
  ) puzzles
) sq
where anagram in (select word from words)
group by word
order by wcount desc, word
limit 10;
\timing

-- What puzzles have the least words?
\timing
select word, count(distinct anagram) wcount from
(
  select word, gen_anagram(word) over (partition by word)
  from 
  (
    select word from words where length(word) = 7
  ) puzzles
) sq
where anagram in (select word from words)
group by word
order by wcount, word
limit 10;
\timing

-- What puzzle has the most total letters (score)?
\timing
select word, sum(length(anagram)) score, count(*) words from
(
  select word, anagram from
  (
    select word, gen_anagram(word) over (partition by word)
    from 
    (
      select word from words where length(word) = 7
    ) puzzles
  ) candidates
  where anagram in (select word from words)
  group by word, anagram
) solutions
group by word
order by score desc, word
limit 10;
\timing

-- What puzzle has the least total letters (score)?
select word, sum(length(anagram)) score, count(*) words from
(
  select word, anagram from
  (
    select word, gen_anagram(word) over (partition by word)
    from 
    (
      select word from words where length(word) = 7
    ) puzzles
  ) candidates
  where anagram in (select word from words)
  group by word, anagram
) solutions
group by word
order by score, word
limit 10;

-- Are all 6-letter words covered by an anagram of a 7-letter word?
\timing
select word from words where word not in
(
   select anagram from
   (
       select gen_anagram(word) over (partition by word) from
       (select word from words where length(word) = 7) word7
   ) anagrams
   where anagram in (select word from words)
   and length(anagram) < 7
)
and length(word) = 6
order by word;
\timing

-- Which word appears in the most puzzles?
\timing
select anagram, count(*) acount from
(
  select distinct word, anagram from
  (
    select word, gen_anagram(word) over (partition by word)
    from 
    (
      select word from words where length(word) = 7
    ) puzzles
  ) solutions
  where anagram in (select word from words)
) frequency
group by anagram
order by acount desc, anagram
limit 10;
\timing

-- Which puzzles don't have a 6-letter word?
\timing
select count(*) from words where word not in
(
  select distinct word from
  (
    select word, gen_anagram(word) over (partition by word)
    from 
    (
      select word from words where length(word) = 7
    ) puzzles
  ) solutions
  where anagram in (select word from words)
  and length(anagram) = 6
)
and length(word) = 7;
\timing

drop library AnagramLib cascade;
drop table words cascade;
