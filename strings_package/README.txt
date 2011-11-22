***************
* Vertica UDF Strings package
*
* This library contains commonly used string manipulation functions
* for use in text search and natural language processing applications
*
* StringTokenizer: Tokenizes strings on spaces into single words
*
* EditDistance: How many characters words differ by (http://en.wikipedia.org/wiki/Levenshtein_distance)
*   Contributed by UbiSoft who says: "On a 4-node cluster, it achieved
*   an impressive performance of 100M rows a second (about 15 seconds
*   to go through 1.5B rows)."
*
*
* Stemmer: the famous Porter Stemmer (http://en.wikipedia.org/wiki/Stemming)
*
* NGrams:  find all 2,3,4 and 5 word sequences in sentences
*          (http://en.wikipedia.org/wiki/N-gram).
* 
* WordCount: Counts the number of words that appear in a sentence
*
***************

1) To build, run:

  cd build
  make

2) to install, run:

./install.sh

3) to uninstall, execute the following sql:

./uninstall.sh
