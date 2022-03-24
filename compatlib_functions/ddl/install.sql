select version();

\set libfile '\''`pwd`'/lib/CompatLib.so\'';
CREATE LIBRARY CompatLib AS :libfile;

CREATE TRANSFORM FUNCTION connect_by_path AS LANGUAGE 'C++' NAME 'ConnectByIntFactory' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION connect_by_path AS LANGUAGE 'C++' NAME 'ConnectByVarcharFactory' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION transpose       AS LANGUAGE 'C++' NAME 'TransposeFactory' LIBRARY CompatLib;

-- Add more as needed
CREATE TRANSFORM FUNCTION group_generator_3 AS LANGUAGE 'C++' NAME 'GroupGeneratorFactoryVVV' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION group_generator_3 AS LANGUAGE 'C++' NAME 'GroupGeneratorFactoryFVF' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION group_generator_4 AS LANGUAGE 'C++' NAME 'GroupGeneratorFactoryVVVV' LIBRARY CompatLib;
CREATE TRANSFORM FUNCTION group_generator_4 AS LANGUAGE 'C++' NAME 'GroupGeneratorFactoryFVFV' LIBRARY CompatLib;
