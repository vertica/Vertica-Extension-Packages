-- See README.md to see what this script is all about

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
