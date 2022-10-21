\set HIGHPASS_CSV ''''`pwd`'/highpass_data.csv'''
\set library highpasslib
\set libfile ''''`pwd`'/highpass.py'''

drop library :library cascade;
CREATE LIBRARY :library AS :libfile LANGUAGE 'Python';
CREATE FUNCTION highpass AS LANGUAGE 'Python' NAME 'highpass_factory' LIBRARY :library fenced;

--- Load the data from the csv file
DROP TABLE IF EXISTS highpass_data cascade;
DROP TABLE IF EXISTS highpass_data_with_deltas cascade;

CREATE FLEX TABLE highpass_data();
COPY highpass_data 
FROM 
:HIGHPASS_CSV
PARSER fcsvparser();
SELECT compute_flextable_keys_and_build_view('highpass_data');

--- convert the raw data to a more convenient form
--- in particular, calculate the delta-t and delta-reading between rows 
create table highpass_data_with_deltas (row_n, tstamp, dt, reading, dreading, nLap)
as select row_number() over (order by tstamp), 
          tstamp, 
          (tstamp - lag(tstamp, 1) over (order by tstamp)) / interval '1 second' seconds,
          reading, 
          reading - lag(reading, 1) over (order by tstamp),
          nLap
from highpass_data_view;


