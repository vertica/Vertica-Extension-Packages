-- W3C Parser
create transform function w3cLogParser
as language 'C++' name 'w3cLogParserFactory' library w3cLogParser;

CREATE TABLE raw_w3c_logs (id AUTO_INCREMENT, data VARCHAR(4096), filename VARCHAR(1024));
COPY raw_w3c_logs (
   data,
   filename as 'w3c.log'
)
FROM STDIN DELIMITER '|';
#Fields: date time c-ip s-sitename s-ip s-port cs-uri-stem cs-uri-query sc-status
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38 - 302
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/AutdState.xml - 200
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/FolderSyncFile - 200
#Fields: date time s-sitename s-ip cs-method cs-uri-stem cs-uri-query s-port cs-username c-ip cs(User-Agent) sc-status sc-substatus sc-win32-status
2011-10-29 17:35:56 W3SVC1 192.168.50.20 GET /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38 - 80 MYCORPjoedoe 192.168.50.20 Microsoft-Server-ActiveSync/6.5.7653.5 302 0 0
2011-10-29 17:35:56 W3SVC1 192.168.50.20 GET /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/AutdState.xml - 80 MYCORPjoedoe 192.168.50.20 Microsoft-Server-ActiveSync/6.5.7653.5 200 0 0
2011-10-29 17:35:56 W3SVC1 192.168.50.20 GET /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/FolderSyncFile - 80 MYCORPjoedoe 192.168.50.20 Microsoft-Server-ActiveSync/6.5.7653.5 200 0 0
#Fields: date time c-ip s-sitename s-ip s-port cs-uri-stem cs-uri-query sc-status
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38 - 302
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/AutdState.xml - 200
2011-10-29 17:35:56 192.168.50.20 W3SVC1 192.168.50.20 80 /exchange/joedoe@example.com/NON_IPM_SUBTREE/Microsoft-Server-ActiveSync/iPad/ApplGB022BTTZ38/FolderSyncFile - 200
\.

-- Invoke the UDT to parse each log line into
CREATE TABLE parsed_w3c_logs AS
SELECT *
FROM (SELECT w3cLogParser(data) OVER (PARTITION BY filename ORDER BY id)
      FROM raw_w3c_logs) AS sq;

-- Show the parsed log lines
SELECT * FROM parsed_w3c_logs LIMIT 10;

SELECT date, c_ip, count(distinct(cs_user_agent)) 
FROM parsed_w3c_logs 
GROUP BY date, c_ip
LIMIT 10;

DROP TABLE raw_w3c_logs;
DROP TABLE parsed_w3c_logs;
