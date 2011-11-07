
-- Invoke the UDT to parse each log line into 
CREATE TABLE parsed_logs AS
SELECT
-- The following expression swizzles apache timestamp column 
-- into a format vertica can parse into a timestamptz
(substr(ts_str, 4, 3) || ' ' || 
 substr(ts_str, 1, 2) || ', '|| 
 substr(ts_str, 8, 4) || ' ' || 
 substr(ts_str, 13) || ' ')::timestamptz as ts,
* -- all other fields
FROM  (SELECT ApacheParser(data) OVER (PARTITION BY substr(data,1,20))  -- partition by prefix
       FROM raw_logs) AS sq;


\echo **********************
\echo ** Example parsed logs
\echo **********************
SELECT * FROM parsed_logs;

\echo **********************
\echo ** Example decoding search terms encoded in URIs
\echo **********************

-- Extract the URIs from the google searches
SELECT request_url, value as search_term
FROM 
  (SELECT request_url, UriExtractor(referring_url) OVER (PARTITION BY request_url) FROM parsed_logs ) AS sq
WHERE sq.name = 'q';

