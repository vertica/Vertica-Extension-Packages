-- (c) Copyright [2022] Micro Focus or one of its affiliates.
-- Licensed under the Apache License, Version 2.0 (the "License");
-- You may not use this file except in compliance with the License.
-- You may obtain a copy of the License at

-- http://www.apache.org/licenses/LICENSE-2.0

-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

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


