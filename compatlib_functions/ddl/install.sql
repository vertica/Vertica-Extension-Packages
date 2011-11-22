select version();

\set libfile '\''`pwd`'/lib/CompatLib.so\'';
CREATE LIBRARY CompatLib AS :libfile;

CREATE TRANSFORM FUNCTION connect_by_path AS LANGUAGE 'C++' NAME 'ConnectByFactory' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION transpose       AS LANGUAGE 'C++' NAME 'TransposeFactory' LIBRARY CompatLib;

