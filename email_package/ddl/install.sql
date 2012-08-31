select version();

\set libfile '\''`pwd`'/build/myudx.so\'';

CREATE LIBRARY EMAILLib as :libfile;
CREATE FUNCTION EMAIL as language 'C++' name 'EMAILFactory' library EMAILLib;