select version();

\set libfile '\''`pwd`'/lib/CompatLibFunctions.so\'';
CREATE LIBRARY CompatLibFunctions AS :libfile;

CREATE TRANSFORM FUNCTION connect_by_path AS LANGUAGE 'C++' NAME 'ConnectByFactory' LIBRARY CompatLibFunctions;
CREATE TRANSFORM FUNCTION transpose       AS LANGUAGE 'C++' NAME 'TransposeFactory' LIBRARY CompatLibFunctions;

