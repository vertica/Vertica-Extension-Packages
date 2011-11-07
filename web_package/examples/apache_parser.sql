
/*** Apache Log Parser Example 3 ***/
create transform function ApacheParser 
as language 'C++' name 'ApacheParserFactory' library TransformFunctions;

CREATE TABLE raw_logs (data VARCHAR(4000));
COPY raw_logs FROM STDIN;
217.226.190.13 - - [16/Oct/2003:02:59:28 -0400] "GET /scripts/nsiislog.dll" 404 307
65.124.172.131 - - [16/Oct/2003:03:50:51 -0400] "GET /scripts/nsiislog.dll" 404 307
65.194.193.201 - - [16/Oct/2003:09:17:42 -0400] "GET / HTTP/1.0" 200 14
66.92.74.252 - - [16/Oct/2003:09:43:49 -0400] "GET / HTTP/1.1" 200 14
66.92.74.252 - - [16/Oct/2003:09:43:49 -0400] "GET /favicon.ico HTTP/1.1" 404 298
65.221.182.2 - - [01/Nov/2003:22:39:51 -0500] "GET /main.css HTTP/1.1" 200 373
65.221.182.2 - - [01/Nov/2003:22:39:52 -0500] "GET /about.html HTTP/1.1" 200 532
65.221.182.2 - - [01/Nov/2003:22:39:55 -0500] "GET /web.mit.edu HTTP/1.1" 404 298
66.249.67.20 - - [02/May/2011:03:28:35 -0700] "GET /robots.txt HTTP/1.1" 404 335 "-" "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
66.249.67.20 - - [02/May/2011:03:28:35 -0700] "GET /classes/commit/fft-factoring.pdf HTTP/1.1" 200 69534 "-" "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
123.108.250.82 - - [02/May/2011:19:59:17 -0700] "GET /classes/commit/pldi03-aal.pdf HTTP/1.1" 200 346761 "-" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)"
\.

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

-- Show the parsed log lines
SELECT * FROM parsed_logs;

DROP TABLE raw_logs;
DROP TABLE parsed_logs;

