select version();

\set libfile '\''`pwd`'/lib/NthValue.so\'';
CREATE LIBRARY NthValue AS :libfile;

CREATE ANALYTIC FUNCTION NTH_VALUE AS LANGUAGE 'C++' NAME 'NthValueFactory' LIBRARY NthValue;
