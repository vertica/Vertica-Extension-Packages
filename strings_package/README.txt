-------------------------------
INTRODUCTION
-------------------------------

This library contains commonly used string manipulation functions for
use in text search and natural language processing applications

StringTokenizer: Tokenizes strings on spaces into single words

EditDistance: How many characters words differ by
(http://en.wikipedia.org/wiki/Levenshtein_distance)

Stemmer: the famous Porter Stemmer
(http://en.wikipedia.org/wiki/Stemming)

NGrams: find all 2,3,4 and 5 word sequences in sentences
(http://en.wikipedia.org/wiki/N-gram).
 
WordCount: Counts the number of words that appear in a sentence

GenAnagram: For each input string, it produces many output strings, one for each
permutation of every subset of letters. (doc/Anagramarama.pdf)

GroupConcat: Concatenates strings in a partition into a comma-separated list.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install this library by running the SQL statements in
 src/ddl/install.sql 
or, to uninstall,
 src/ddl/uninstall.sql
Note that the SQL statements assume that you have copied this package to a
node in your cluster and are running them from there.

Alternatively, assuming vsql is in your path, just do:

$ make install
$ make uninstall

-------------------------------
BUILDING
-------------------------------

To build from source:

$ make


-------------------------------
USAGE
-------------------------------

Please see examples

-------------------------------
PERFORMANCE
-------------------------------

EditDistance: Contributed by UbiSoft who says: "On a 4-node cluster,
it achieved an impressive performance of 100M rows a second (about 15
seconds to go through 1.5B rows)."

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
