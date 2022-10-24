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

DROP TABLE IF EXISTS lowpass_out CASCADE;
CREATE TABLE lowpass_out AS 
SELECT row_n, tstamp, reading, dt, dreading,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 0.001) as f001,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 0.0016) as f0016,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 0.002) as f002,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 0.01) as f01,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 1) as f1,
       p_lowpass(dt, reading USING PARAMETERS "frequency" = 5) as f5,
       p_lowpass(dt, reading USING PARAMETERS "alpha" = .00001) as a00001,
       p_lowpass(dt, reading USING PARAMETERS "alpha" = .0001) as a0001,
       p_lowpass(dt, reading USING PARAMETERS "alpha" = .001) as a001,
       p_lowpass(dt, reading USING PARAMETERS "alpha" = .010) as a010,
       p_lowpass(dt, reading USING PARAMETERS "alpha" = .1) as a100
FROM lowpass_data_with_deltas;
