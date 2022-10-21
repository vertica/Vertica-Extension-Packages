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
       
