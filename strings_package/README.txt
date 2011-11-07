***************
** Example Vertica UDF Strings package
**
** This library contains commonly used string manipulation functions
**
** StringTokenizer: Tokenizes strings on spaces into single words (example transform)
**
** EditDistance: Contributed by UbiSoft:
**
** Notes: I implemented the edit distance function as a Vertica UDF:
** http://en.wikipedia.org/wiki/Levenshtein_distance. On a 4-node
** cluster, it achieved an impressive performance of 100M rows a second
** (about 15 seconds to go through 1.5B rows).
**
***************

***************
** Other Functions we would like to see implemented:
**
** A Stemmer (http://en.wikipedia.org/wiki/Stemming) 
**   -- likely the famous Porter Stemmer (http://tartarus.org/~martin/PorterStemmer/)
**
***************

1) To build, run:

  cd build
  make

2) to install, run:

./install.sh

3) to uninstall, execute the following sql:

./uninstall.sh
