-------------------------------
INTRODUCTION
-------------------------------

This package provides the following functionality:

1. Twitter Search - perform a search on Twitter and return tweet texts as
results
2. Sentiment analysis - Simple algorithm to provide a sentiment score to text.
This can be used over tweets to find general user sentiments.
3. Word stemmer - uses the Porter stemming algorithm to stem words (e.g. going
-> go etc.). Useful in sentiment analysis to canonicalize words, but also
useful by itself
4. URL Fetcher - function that fetches the given URL as a text string. Due to
limitations in Vertica's varchar size, fetches only the first 65000 bytes of
the URL.
5. Twitter JSON Parser - function that parses the Twitter JSON format and
returns tweet texts

-------------------------------
BUILDING
-------------------------------

To build:

$ make

For non-standard SDK locations, use

$ make SDK=/path/to/sdk

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

Assuming vsql is in your path, just do:

$ make install
$ make uninstall

Alternately, you can find the DDL that 'make install' uses in:
 src/ddl/install.sql 
and
 src/ddl/uninstall.sql

-------------------------------
USAGE
-------------------------------

See examples/demo.sql for examples of usage. Brief usage summary:

stem: Scalar function
  Input - word to the stemmed
  Output - word stem

fetch_url: Scalar function 
  Input - the URL to fetch
  Output - the contents of the URL

sentiment: Scalar function
  Input - Sentence/text
  Output - Sentiment score - positive numbers indicate positive sentiment, 0
           indicates neutral, and negative numbers indicate negative 
           sentiment

twitter: Transform function
  Input - Twitter JSON data
  Output - Twitter query (if any), and tweet texts contained in the JSON input
           data

twitter_search: Transform function
  Input - Twitter search query 
  Output - The query, and tweet texts matching the query

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
