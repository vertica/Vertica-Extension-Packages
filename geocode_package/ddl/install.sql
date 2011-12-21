-- Installaction script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/Geocode.so\'';

CREATE LIBRARY GeocodeLib as :libfile;
CREATE FUNCTION geocode as language 'C++' name 'GeocodeFactory' library GeocodeLib;


