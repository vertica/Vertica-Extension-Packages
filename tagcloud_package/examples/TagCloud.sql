\set textCorpus '\''`pwd`'/examples/text_corpus.txt\''
\set htmlFile '\''`pwd`'/output.html\''

\echo ***************** Search 'vertica' in the small text_corpus.txt ***************** 
select RelevantWordsNoLoad('vertica', :textCorpus) over() order by weight desc limit 20;



\echo ***************** Load text_corpus.txt into a table first, and then search 'vertica' in it ***************** 
create table alltext(line varchar(64000));
copy alltext(line) from :textCorpus DELIMITER E'\n';

select RelevantWords('vertica', line) over() from alltext order by weight desc limit 20;

drop table alltext cascade;



\echo ****************************** Generate HTML to show the graphical effect *************************
\echo ****** This generates output.html in current direcotry, use your favoriate browser to see it ******
drop table words cascade;
create table words(weight float, word varchar);
insert into words select RelevantWordsNoLoad('vertica', :textCorpus) over() order by weight desc limit 50;
select GenerateTagCloud(weight, word, :htmlFile) over () from words;
