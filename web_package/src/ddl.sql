select version();

\set libfile '\''`pwd`'/lib/WebPackage.so\'';
CREATE LIBRARY WebPackage AS :libfile;

CREATE TRANSFORM FUNCTION ApacheParser AS LANGUAGE 'C++' NAME 'ApacheParserFactory' LIBRARY WebPackage;
CREATE TRANSFORM FUNCTION UriExtractor AS LANGUAGE 'C++' NAME 'UriExtractorFactory' LIBRARY WebPackage;
CREATE TRANSFORM FUNCTION w3cLogParser AS LANGUAGE 'C++' NAME 'w3cLogParserFactory' LIBRARY WebPackage;

