-- Installation script: defined the shared library and the appropriate entry points

select version();

\set libfile '\''`pwd`'/lib/TwitterSentiment.so\'';

CREATE LIBRARY TwitterSentimentLib as :libfile;
CREATE FUNCTION stem as language 'C++' name 'StemFactory' library TwitterSentimentLib;
CREATE FUNCTION fetch_url as language 'C++' name 'FetchURLFactory' library TwitterSentimentLib;
CREATE FUNCTION sentiment as language 'C++' name 'SentimentFactory' library TwitterSentimentLib;
CREATE TRANSFORM FUNCTION twitter as language 'C++' name 'TwitterJSONFactory' library TwitterSentimentLib;
CREATE TRANSFORM FUNCTION twitter_search as language 'C++' name 'TwitterSearchFactory' library TwitterSentimentLib;



