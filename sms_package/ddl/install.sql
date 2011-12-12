-- Installaction script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/SMS.so\'';

CREATE LIBRARY SMSLib as :libfile;
CREATE FUNCTION SMS as language 'C++' name 'SMSFactory' library SMSLib;


