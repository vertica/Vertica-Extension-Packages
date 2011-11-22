-- See doc/Anagramarama.pdf for a complete discussion

drop table if exists words cascade;
create table words(word varchar(8));

\set datafile '\''`pwd`'/test-data/wordlist.txt\'';
copy words from :datafile;

\echo ************************
\echo How many words total?
\echo ************************

select count(*) from words;

\echo ************************
\echo How many words have 7 letters?  This is the number of puzzles there are.
\echo ************************
--  (Actually there are less, as 7-letter words may be anagrams of each
--   other.  More on this later.)
-- There are 13699 ways to arrange the letters in a 7-letter word
--  (not taking duplicates into account, but including shorter words).
select count(*) from words where length(word) = 7;

\echo ************************
\echo A simple test - the first puzzle
\echo ************************

select word from words where length(word) = 7 order by word limit 1;

\echo ************************
\echo What are the real words that are in this puzzle?
\echo ************************

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

\echo ************************
\echo What puzzles have the most words?
\echo ************************

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

\echo ************************
\echo What puzzles have the least words?
\echo ************************
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

\echo ************************
\echo What puzzle has the most total letters (score)?
\echo ************************

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

\echo ************************
\echo What puzzle has the least total letters (score)?
\echo ************************

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

\echo ************************
\echo Are all 6-letter words covered by an anagram of a 7-letter word?
\echo ************************

\timing
select count(word) from words where word not in
(
   select anagram from
   (
       select gen_anagram(word) over (partition by word) from
       (select word from words where length(word) = 7) word7
   ) anagrams
   where anagram in (select word from words)
   and length(anagram) < 7
)
and length(word) = 6;
\timing

\echo ************************
\echo Which word appears in the most puzzles?
\echo ************************

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

\echo ************************
\echo Which puzzles do not have a 6-letter word?
\echo ************************

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

\echo ************************
\echo How many distinct puzzles are there?
\echo ************************
-- We find the number of 7-letter words whose 7-letter anagram that
--  is first alphabetically is equal to the word.  (If not, some other
--  word generates the same puzzle.)
\timing
select count(*)
from
(
  select word, anagram
  from
  (
    select word, anagram, row_number() over(partition by word order by anagram) rn
    from
    (
      select word, anagram
      from
      (
        select word, gen_anagram(word) over (partition by word)
        from 
        (
          select word from words where length(word) = 7
        ) puzzles
      ) sq2
      where length(anagram) = 7 
      and anagram in (select word from words)
    ) l7
  ) sq3
  where rn = 1
) sq
where word = anagram;
\timing

drop table words cascade;
