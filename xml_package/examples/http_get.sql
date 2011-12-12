----------------
-- Example of getting URL data
----------------
create table raw_data(data varchar(64000), url varchar(64000));

-- fetch Vertica.com's homepage into the table
insert into raw_data select http_get('http://www.vertica.com'), 'http://www.vertica.com';
COMMIT;

select length(data) as len, url from raw_data;

DROP TABLE raw_data;