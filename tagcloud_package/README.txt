-------------------------------
INTRODUCTION
-------------------------------

This library contains functions to generate Tag Cloud for a given key word
with in a provided text corpus. When using Wikipedia as the text corpus, it
achives similar effect as a previous Hadoop implementation:
http://www.youtube.com/watch?feature=player_detailpage&v=2Iz5V9MrkBg#t=120s

Two types of functions are implemented to achieve the goal:

The first type is to gather the relevant words and their relevance scores to the
key word, RelevantWords or RelevantWordsNoLoad can be used for the purpose when
the text corpus is already loaded into Vertica, or the text corpus is an
external file respectively.

The second one is generate Tag Cloud in HTML taking the words and their
relevance scores, which is the output of the previous function. The function
name is GenerateTagCloud.

See examples/example_tag_cloud.html as an example of the result in visual
effect when 'vertica' is use as the key word and the whole wikipedia is used as
searched corpus (download available at
http://en.wikipedia.org/wiki/Wikipedia:Database_download)


-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install this library by running the SQL commands in
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

To build:

$ make


-------------------------------
USAGE
-------------------------------

RelevantWords('key_word', 'text_columns')

Arguments:
key_word     - the search key word
text_columns - the varchar columns containing text corpus, there is no
               restriction about how the column is orgnized/ordered, the
               function just treats the input as a stream of incoming words

Output columns:
weight       - the relevance score of the word
word         - the words that the algorithm considers relevant to the key word



RelevantWordsNoLoad('key_word', 'corpus_file_name')

Arguments:
key_word         - the search key word
corpus_file_name - the file name of the text corpus, this function is helpful
                   when the corpus data is not loaded into Vertica

Output columns:
Same as RelevantWords()


GenerateTagCloud('score', 'word', 'html_file_name')

Arguments:
sore           - the relevance score of the word from RelevantWordsNoLoad()
                 or RelevantWords()
word           - the relevant word
html_file_name - the file name to for the generated HTML file

Output columns:
status         - the status of HTML file generation

-------------------------------
PERFORMANCE
-------------------------------

The function is relatively disk IO heavy. On a laptop, using the whole 33G
uncompressed wikipedia as the text corpus, it finishes in about 6~7 minutes
with disk utility above 90% , as a comparison simply 'cat' the text corpus
into /dev/null also taks a little bit over 6 minutes.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
