import math
import vertica_sdk

"""
An implementation of the High-pass filter as described by
https://en.wikipedia.org/wiki/High-pass_filter 

Setup:

--- unfortunately, these filenames need to be an absolute pathname
\set libfile '\'highpass.py\''
--- also needs to be an absolute pathname
\set data_csv '\'maw_tbrake_202104291444.csv\''

drop library highpasslib cascade;
CREATE LIBRARY highpasslib AS :libfile LANGUAGE 'Python';
CREATE FUNCTION highpass AS LANGUAGE 'Python' NAME 'highpass_factory' LIBRARY highpasslib fenced;

--- this only needs to be done once
create flex table f_brake_data();
copy f_brake_data from :data_csv parser fcsvparser();
select compute_flextable_keys_and_build_view('f_brake_data');

--- create table full of internal representation from view:
create table bd as select tstamp, tbrakediscfl, tbrakediscfr, tbrakediscrl, tbrakediscrr from f_brake_data_view;

--- let's start by creating a table with more convenient column names:
create table b2 (row_n, tstamp, fl, fr, rl, rr) 
as select row_number() over (order by tstamp),
          tstamp, 
          tbrakediscfl, 
          tbrakediscfr, 
          tbrakediscrl, 
          tbrakediscrr
from bd;

--- now let's calculate deltas:
--- https://www.vertica.com/blog/vertica-quick-tip-converting-intervals-numeric/ 
--- teaches us a trick to turn an interval into a numeric by doing arithmetic on it:
--- SELECT INTERVAL '24 HOURS' / INTERVAL '1 DAY' days;
--- (returns 1)
create table b3 (row_n, dt, dfl, dfr, drl, drr)
as select row_n, 
          (tstamp - lag(tstamp, 1) over (order by row_n)) / interval '1 second' seconds,
          fl - lag(fl, 1) over (order by row_n),
          fr - lag(fr, 1) over (order by row_n),
          rl - lag(rl, 1) over (order by row_n), 
          rr - lag(rr, 1) over (order by row_n) 
from b2;

--- Now, we've used SQL to calculate the folhighing:

---  0           1                   2           3           4           5         6        7           8           9           10
select b2.row_n, tstamp, fl, fr, rl, rr, dt, dfl, dfr, drl, drr from b2, b3 where b2.row_n = b3.row_n limit 5;
--- row_n |         tstamp         |    fl     |    fr     |    rl     |    rr     |  dt  |    dfl    |    dfr    |    drl    |    drr    
----------+------------------------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+-----------
---     1 | 2021-04-10 15:29:13.14 | 18.097200 | 16.586560 | 17.990390 | 18.097200 |      |           |           |           |          
---     2 | 2021-04-10 15:29:13.15 | 18.097200 | 16.388190 | 17.395280 | 18.097200 | 0.01 |  0.000000 | -0.198370 | -0.595110 |  0.000000
---     3 | 2021-04-10 15:29:13.16 | 16.891740 | 16.189820 | 17.395280 | 17.898830 | 0.01 | -1.205460 | -0.198370 |  0.000000 | -0.198370
---     4 | 2021-04-10 15:29:13.17 | 16.891740 | 16.189820 | 17.395280 | 17.898830 | 0.01 |  0.000000 |  0.000000 |  0.000000 |  0.000000
---     5 | 2021-04-10 15:29:13.18 | 17.685210 | 16.189820 | 17.288470 | 17.990390 | 0.01 |  0.793470 |  0.000000 | -0.106810 |  0.091560

--- Where: 
--- fl -- front left brake data
--- fr -- front right brake data
--- rl -- rear left brake data
--- rr -- rear right brake data

--- dt -- delta-t (that is, tstamp[row] - tstamp[row-1])
--- dfl -- delta front-left brake data (that is fl[row] - fl[row-1])
--- dfr -- delta front-right brake data
--- drl -- delta rear-left brake data
--- drr -- delta rear-right brake data

--- These are the inputs we need for the high-pass calculation:

\set alpha 10 --- bigger alpha means shigher response to changes
create table bhigh as 
select b2.row_n, tstamp, fl, fr, rl, rr, dt, dfl, dfr, drl, drr, 
       highpass(:alpha, dt, fl) as hp_fl, highpass(:alpha, dt, fr) as hp_fr, highpass(:alpha, dt, rl) as hp_rl, highpass(:alpha, dt, rr) as hp_rr
       from b2, b3 where b2.row_n = b3.row_n and b2.row_n > 1;

select * from bhigh limit 5;
--- row_n |         tstamp         |    fl     |    fr     |    rl     |    rr     |  dt  |    dfl    |    dfr    |    drl    |    drr    |      hp_fl       |      hp_fr       |      hp_rl       |      hp_rr       
----------+------------------------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+-----------+------------------+------------------+------------------+------------------
---(5 rows)

"""

class highpass(vertica_sdk.ScalarFunction):
    """Apply a high-pass filter to a column sorted by timestamp
       For each row, pass in:
        - float freq (a constant)
        - float delta_t
        - float x, the time-series variable being filtered

        Wikipedia describes a high-pass filter here: https://en.wikipedia.org/wiki/High-pass_filter 

        So, the Wikipedia explanation of the algorithm uses a constant
        alpha, derived from the RC-circuit origin of the high-pass
        filter analysis.  But people really want to think about
        frequency, especially if they're not working on an RC circuit.

        Fortunately, the Wikipedia page works this out for us, 
        - freq = 1/(2*pi*RC)
        - alpha = 1/(1 + 2*pi*delta_t*freq)

        In the discrete high-pass filter calculation, 
        
        for t in the time-series:
            y[t] = alpha * (y[t-1] - x[t-1] + x[t])

        The
    """

    def __init__(self):
        self.y_last = 0
        self.x_last = 0
        self.first_row = True

    def setup(self, server_interface, col_types):
        self.y_last = 0
        self.x_last = 0
        self.first_row = True
        server_interface.log("high-pass filter: setup called")

    def processBlock(self, server_interface, arg_reader, res_writer):
        # Writes a string to the UDx log file.
        server_interface.log("Python UDx - high-pass filter!")
        while(True):
            # Example of error checking best practices.
            y = 0
            if arg_reader.isNull(0) or arg_reader.isNull(1) or arg_reader.isNull(2):
                server_interface.log("high-pass filter: Null input")
            else:
                freq = arg_reader.getFloat(0)
                delta_t = arg_reader.getFloat(1)
                val = arg_reader.getFloat(2)
                alpha = 1/(1 + (2 * math.pi * delta_t * freq))
                if self.first_row:
                    server_interface.log("high-pass filter: First row")
                    y = val
                    self.x_last = val
                    self.first_row = False
                else:
                    y = alpha * (self.y_last - self.x_last + val)
                    self.x_last = val
                    self.y_last = y
                                 
            res_writer.setFloat(y)
            res_writer.next() # Read the next row.
            if not arg_reader.next():
                # Stop processing when there are no more input rows.
                server_interface.log("high-pass filter: end of block")
                break
    def destroy(self, server_interface, col_types):
        pass

class highpass_factory(vertica_sdk.ScalarFunctionFactory):
    
    def createScalarFunction(self, srv):
        return highpass()

    def getPrototype(self, srv_interface, arg_types, return_type):
        arg_types.addFloat()
        arg_types.addFloat()
        arg_types.addFloat()
        return_type.addFloat()

    def getReturnType(self, srv_interface, arg_types, return_type):
        return_type.addFloat()
