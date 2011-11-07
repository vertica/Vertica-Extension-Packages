select version();

\set libfile '\''`pwd`'/lib/StringsPackage.so\'';
CREATE LIBRARY StringsLib AS :libfile;

CREATE FUNCTION EditDistance                   AS LANGUAGE 'C++' NAME 'EditDistanceFactory'         LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION StringTokenizer      AS LANGUAGE 'C++' NAME 'StringTokenizerFactory'      LIBRARY StringsLib;
CREATE TRANSFORM FUNCTION StringTokenizerDelim AS LANGUAGE 'C++' NAME 'StringTokenizerDelimFactory' LIBRARY StringsLib;

