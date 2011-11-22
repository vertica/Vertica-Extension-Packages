select version();

\set libfile '\''`pwd`'/lib/StringsLib.so\'';
CREATE LIBRARY StringsLib AS :libfile;

CREATE FUNCTION EditDistance                   AS LANGUAGE 'C++' NAME 'EditDistanceFactory'         LIBRARY StringsLib;
CREATE FUNCTION Stemmer                        AS LANGUAGE 'C++' NAME 'PorterStemmerFactory'        LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION StringTokenizer      AS LANGUAGE 'C++' NAME 'StringTokenizerFactory'      LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION StringTokenizerDelim AS LANGUAGE 'C++' NAME 'StringTokenizerDelimFactory' LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION TwoGrams             AS LANGUAGE 'C++' NAME 'TwoGramsFactory'             LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION ThreeGrams           AS LANGUAGE 'C++' NAME 'ThreeGramsFactory'           LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION FourGrams            AS LANGUAGE 'C++' NAME 'FourGramsFactory'            LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION FiveGrams            AS LANGUAGE 'C++' NAME 'FiveGramsFactory'            LIBRARY StringsLib;
CREATE FUNCTION WordCount                      AS LANGUAGE 'C++' NAME 'WordCountFactory'            LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION gen_anagram          AS LANGUAGE 'C++' NAME 'AnagramFactory'              LIBRARY StringsLib;
