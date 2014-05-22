select version();

\set libfile '\''`pwd`'/lib/JsonPackage.so\'';
CREATE LIBRARY JsonPackage AS :libfile;

CREATE FUNCTION JSON_EXTRACT_PATH_TEXT AS LANGUAGE 'C++' NAME 'JsonExtractPathTextFactory' LIBRARY JsonPackage;
