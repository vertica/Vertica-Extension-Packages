-- Installaction script: defined the shared library and the appropriate entry poings

select version();

\set libfile '\''`pwd`'/lib/TagCloud.so\'';

CREATE LIBRARY TagCloudLib as :libfile;
create transform function RelevantWords as language 'C++' name 'RelevantWordsFactory' library TagCloudLib;
create transform function RelevantWordsNoLoad as language 'C++' name 'RelevantWordsNoLoadFactory' library TagCloudLib;
create transform function GenerateTagCloud as language 'C++' name 'GenerateTagCloudFactory' library TagCloudLib;
