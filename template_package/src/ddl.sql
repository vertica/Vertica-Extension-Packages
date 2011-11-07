select version();

\set libfile '\''`pwd`'/lib/XXXLibName.so\'';
CREATE LIBRARY XXXTemplatePackage AS :libfile;

CREATE TRANSFORM FUNCTION connect_by_path AS LANGUAGE 'C++' NAME 'ConnectByFactory' LIBRARY CompatLibFunctions;

