-- Installaction script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/Template.so\'';

CREATE LIBRARY TemplateLib as :libfile;
CREATE FUNCTION RemoveSpace as language 'C++' name 'RemoveSpaceFactory' library TemplateLib;


