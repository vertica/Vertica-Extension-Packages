\set CSVFILE ''''`pwd`'/lowpass_data.csv'''
\set library lowpasslib
\set libfile ''''`pwd`'/p_lowpass.py'''

DROP LIBRARY IF EXISTS :library CASCADE;
CREATE LIBRARY :library AS :libfile LANGUAGE 'Python';
CREATE FUNCTION lowpass AS LANGUAGE 'Python' NAME 'p_lowpass_factory' LIBRARY :library fenced;

--- Load the data from the csv file
DROP TABLE IF EXISTS lowpass_data cascade;
DROP TABLE IF EXISTS lowpass_data_with_deltas cascade;

CREATE FLEX TABLE lowpass_data();
COPY lowpass_data FROM :CSVFILE PARSER fcsvparser();
SELECT compute_flextable_keys_and_build_view('lowpass_data');
--- create table full of internal representation from view:

--- convert the raw data to a more convenient form
--- in particular, calculate the delta-t and delta-reading between rows 
create table lowpass_data_with_deltas (row_n, tstamp, dt, reading, dreading)
as select row_number() over (order by tstamp), 
          tstamp, 
          (tstamp - lag(tstamp, 1) over (order by tstamp)) / interval '1 second' seconds,
          reading, 
          reading - lag(reading, 1) over (order by tstamp)
from lowpass_data_view;
