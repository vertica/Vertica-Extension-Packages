-- Installation script: defined the shared library and the appropriate entry points

select version();

\set libfile '\''`pwd`'/lib/TextFunctions.so\'';

CREATE LIBRARY TextSearchLib as :libfile;
CREATE FUNCTION stem as language 'C++' name 'StemFactory' library TextSearchLib;


