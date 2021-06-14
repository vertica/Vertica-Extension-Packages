select version();

\set libfile '\''`pwd`'/lib/StringsLib.so\'';
CREATE LIBRARY StringsLib AS :libfile;

CREATE FUNCTION EditDistance                   AS LANGUAGE 'C++' NAME 'EditDistanceFactory'         LIBRARY StringsLib NOT FENCED;
CREATE FUNCTION Stemmer                        AS LANGUAGE 'C++' NAME 'PorterStemmerFactory'        LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION StringTokenizer      AS LANGUAGE 'C++' NAME 'StringTokenizerFactory'      LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION StringTokenizerDelim AS LANGUAGE 'C++' NAME 'StringTokenizerDelimFactory' LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION TwoGrams             AS LANGUAGE 'C++' NAME 'TwoGramsFactory'             LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION ThreeGrams           AS LANGUAGE 'C++' NAME 'ThreeGramsFactory'           LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION FourGrams            AS LANGUAGE 'C++' NAME 'FourGramsFactory'            LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION FiveGrams            AS LANGUAGE 'C++' NAME 'FiveGramsFactory'            LIBRARY StringsLib NOT FENCED;
CREATE FUNCTION WordCount                      AS LANGUAGE 'C++' NAME 'WordCountFactory'            LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION gen_anagram          AS LANGUAGE 'C++' NAME 'AnagramFactory'              LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION group_concat         AS LANGUAGE 'C++' NAME 'GroupConcatFactory'          LIBRARY StringsLib NOT FENCED;
CREATE TRANSFORM FUNCTION group_concat_delim   AS LANGUAGE 'C++' NAME 'GroupConcatDelimFactory'     LIBRARY StringsLib NOT FENCED;