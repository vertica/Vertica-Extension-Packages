
-- Demonstrate functionality of twitter search and sentiment analysis over tweets
create table tweets (query varchar(200), tweet varchar(300));

insert into tweets select twitter_search('#united airlines') over ();
-- twitter gets unhappy if you search too quickly
\! sleep 3
insert into tweets select twitter_search('#american airlines') over ();
\! sleep 3
insert into tweets select twitter_search('#southwest airlines') over ();
\! sleep 3
insert into tweets select twitter_search('#jetblue') over ();
\! sleep 3
insert into tweets select twitter_search('#alaska airlines') over ();
\! sleep 3
insert into tweets select twitter_search('#delta airlines') over ();
commit;

select query, avg(score) from (select query, sentiment(tweet) as score from tweets) sq group by query order by avg(score) desc;

-- demo functionality of other useful functions (building blocks for above)

-- sentiment function
select sentiment('i feel full');
select sentiment('i feel awful and terrible and horrible');
select sentiment('i feel fantastic and awesome');

-- twitter search function
select twitter_search('vertica') over ();
select twitter_search('#hp') over ();

-- function to fetch a URL
select fetch_url('http://search.twitter.com/search.json?page=1&max_id=145229360729817088&q=united%20airlines');

-- parse twitter json format
select twitter(tw) over () from (select fetch_url('http://search.twitter.com/search.json?page=1&max_id=145229360729817088&q=united%20airlines') as tw) sq;

-- word stemmer
select stem('going');
select stem('goes');
select stem('feeling');
select stem('unbelievably');


-- clean up
drop table tweets;
