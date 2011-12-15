-- Installation script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/Shell.so\'';

CREATE LIBRARY ShellLib as :libfile;
CREATE FUNCTION hostname as language 'C++' name 'HostnameFactory' library ShellLib;
CREATE TRANSFORM FUNCTION shell_execute as language 'C++' name 'ShellFactory' library ShellLib;
CREATE TRANSFORM FUNCTION int_sequence as language 'C++' name 'IntSequenceFactory' library ShellLib;

\set tablename allnodes
\set istemp
\set nodes 'all nodes'
\i ddl/create-dist-table.sql
