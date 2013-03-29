\set libfile '\''`pwd`'/build/ODBCLoader.so\''
CREATE LIBRARY ODBCLoaderLib AS :libfile;
CREATE PARSER ODBCLoader AS LANGUAGE 'C++' NAME 'ODBCLoaderFactory' LIBRARY ODBCLoaderLib;
CREATE SOURCE ODBCSource AS LANGUAGE 'C++' NAME 'ODBCSourceFactory' LIBRARY ODBCLoaderLib;