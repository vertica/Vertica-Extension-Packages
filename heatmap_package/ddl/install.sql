-- Installaction script: defined the shared library and the appropriate entry poings

select version();

-- Step 1: Create LIBRARY 
\set libfile '\''`pwd`'/lib/HeatMapLib.so\'';
CREATE LIBRARY HeatMapFunctions AS :libfile;

-- Step 2: Create Functions
CREATE TRANSFORM FUNCTION heat_map AS LANGUAGE 'C++'
NAME 'HeatMapFactory' LIBRARY HeatMapFunctions;

CREATE TRANSFORM FUNCTION heat_map_image AS LANGUAGE 'C++'
NAME 'HeatMapImageFactory' LIBRARY HeatMapFunctions;