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


DROP TABLE IF EXISTS highpass_out;
CREATE TABLE highpass_out AS
SELECT highpass_data_with_deltas.row_n as row_n,
       highpass_data_with_deltas.reading as reading,
       highpass(5, dt, reading) as f5,
       highpass(10, dt, reading) as f10,
       highpass(100, dt, reading) as f100,
       highpass(1000, dt, reading) as f1000,
       highpass(10000, dt, reading) as f10000
FROM highpass_data_with_deltas
WHERE highpass_data_with_deltas.row_n > 1
ORDER BY highpass_data_with_deltas.row_n;
       
