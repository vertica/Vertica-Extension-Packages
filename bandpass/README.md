
# Table of Contents

1.  [Python Bandpass filters](#org4b48181)
2.  [Installation](#org6806c21)
    1.  [Execute the following scripts from the source directory](#orga217a61)
        1.  [Loading the data into Vertica from the CSV file](#org17e3e15)
    2.  [Loading the libraries](#org51c3bd0)
        1.  [Loading the lowpass library](#org20014ee)
        2.  [Loading the highpass library](#org23719b7)
    3.  [Running the functions](#org39510f2)
        1.  [p_lowpass](#org9632d7d)
        2.  [highpass](#org5135c50)
    4.  [Plotting the result](#org58092cc)
3.  [High pass filter](#orgfb758bf)
    1.  [Running the highpass filter on the low-pass data](#orgb4c7aff)
    2.  [Running the highpass filter on the highpass data](#org12f0c5c)
4.  [Comparison of these filters to filtering with Fourier Transforms](#orgfd8a3a1)


<a id="org4b48181"></a>

# Python Bandpass filters

This repository contains Python user-defined functions (UDFs) for
highpass and low-pass filters on time-series data.

The algorithms implemented are drawn from the descriptions of the
discrete-time versions described in Wikipedia: 

-   <https://en.wikipedia.org/wiki/Low-pass_filter>
-   <https://en.wikipedia.org/wiki/High-pass_filter>


<a id="org6806c21"></a>

# Installation


<a id="orga217a61"></a>

## Execute the following scripts from the source directory

The following scripts use `pwd` to set the absolute pathname of
`lowpass_data.csv`, `highpass_data.csv`,the `lowpass.py` library, and
the `highpass.py` library.  Please execute them in the directory where
those files are located.


<a id="org17e3e15"></a>

### Loading the data into Vertica from the CSV file

(Note: the following SQL scripts presume that your Vertica has
flextables installed.)

For historical reasons we have two data-sets intended for the
different filters &#x2014; `highpass.csv` and `lowpass.csv`.  The
algorithms can be run on either dataset, of course.

    vsql -f lowpass_load.sql
    vsql -f highpass_load.sql

`lowpass_load.sql` will load the data from `lowpass_data.csv` into a flextable,
`lowpass_data`.  It then derives a standard Vertica table,
`lowpass_data_with_deltas`, which has columns:

-   **row_n:** The row number of the data
-   **tstamp:** The timestamp from the data (`row_n` and `tstamp` may be
    redundant, as they have the same ordering).
-   **dt:** the "delta" of this row's timestamp from the previous row's
    timestamp.  Used as the "delta-t" in the discrete low- and
    highpass filter algorithms.
-   **reading:** the sensor reading for this time
-   **dreading:** the "delta" of this reading from the previous row's
    reading.  Used as the "delta-signal" in the discrete
    low- and highpass filter algorithms.

`highpass_load.sql` does the same thing for the highpass data in
`highpass.csv`, creating:
`lowpass_data`.  It then derives a standard Vertica table,
`highpass_data_with_deltas`, which has similar columns to
`lowpass_data_with_deltas`: 

-   **row_n:** The row number of the data
-   **tstamp:** The timestamp from the data (`row_n` and `tstamp` may be
    redundant, as they have the same ordering).
-   **dt:** the "delta" of this row's timestamp from the previous row's
    timestamp.  Used as the "delta-t" in the discrete low- and
    highpass filter algorithms.
-   **reading:** the sensor reading for this time
-   **dreading:** the "delta" of this reading from the previous row's
    reading.  Used as the "delta-signal" in the discrete
    low- and highpass filter algorithms.


<a id="org51c3bd0"></a>

## Loading the libraries

The libraries are loaded by the first few lines of `lowpass_load.sql`
and `highpass_load.sql`.  These two SQL scripts must be run in the
directory containing `p_lowpass.py` and `highpass.py`.


<a id="org20014ee"></a>

### Loading the lowpass library

We inform Vertica about the file to load the library from by giving it
an absolute pathname to the library, then informing Vertica how to
find the function-factory within the library.

Run these commands in the directory containing `p_lowpass.py`:

    \set library lowpasslib
    \set libfile ''''`pwd`'/p_lowpass.py'''
    
    DROP LIBRARY IF EXISTS :library CASCADE;
    CREATE LIBRARY :library AS :libfile LANGUAGE 'Python';
    CREATE FUNCTION lowpass 
        AS LANGUAGE 'Python' NAME 'p_lowpass_factory' 
        LIBRARY :library fenced;


<a id="org23719b7"></a>

### Loading the highpass library

The commands for loading the highpass library are similar to those
used to load the lowpass library.  

Run these commands in the directory containing `lowpass.py`:

    \set library highpasslib
    \set libfile ''''`pwd`'/highpass.py'''
    
    drop library :library cascade;
    CREATE LIBRARY :library AS :libfile LANGUAGE 'Python';
    CREATE FUNCTION highpass AS LANGUAGE 'Python' NAME 'highpass_factory' LIBRARY :library fenced;


<a id="org39510f2"></a>

## Running the functions

There are two files, `highpass_run.sql` and `lowpass_run.sql` which
can be used to run the libraries.


<a id="org9632d7d"></a>

### p_lowpass

`lowpass_run.sql` runs the lowpass algorithm across several sets of
parameters for frequency and for "alpha".  The alpha parameter is
meaningful in the context of the viewing the lowpass filter as an RC
circuit (alpha = RC/(RC + delta_t)).

The interface for the lowpass function is:

    p_lowpass(
        <float-delta-time>, 
        <float-sensor-reading> 
        USING PARAMETERS "frequency" = <float-frequency-in-hz>
    ) ORDER BY timestamp;
    --- or
    p_lowpass(
        <float-delta-time>, 
        <float-sensor-reading> 
        USING PARAMETERS "alpha" = <float-alpha>
    ) ORDER BY timestamp;

It returns a float value for each row representing the output of the
lowpass filter at that point.

Where:

-   **<float-delta-time>:** the time-interval between this sensor
    reading and the previous one in seconds (e.g., 0.01)
-   **<float-sensor-reading>:** the sensor reading in whatever units
    you're measuring
-   **<float-frequency-in-hz>:** the cutoff frequency for the filter in
    hertz
-   **<float-alpha>:** 0 < alpha < 1

**alpha** can be defined in terms of frequency, **f** and **delta_T**:

alpha = delta_T / (RC + delta_T)

alpha = 2\*pi\*delta_T\*f/((2\*pi\*delta_T\*f) + 1)

Similarly, **f** can ge defined in terms of **alpha** and **delta_T**:

f = alpha/((1-alpha)\*2\*pi\*delta_T)


<a id="org5135c50"></a>

### highpass

`highpass_run.sql` runs the highpass algorithm on the
`highpass_data_with_deltas` table created by `highpass_load.sql`,
\*using different cutoff frequencies.

The interface for the highpass function is:

    highpass(<float-frequency>, <float-delta-time>, <float-sensor-reading>) 
    ORDER BY timestamp;

Where:

-   **<float-frequency>:** the cutoff frequency in hertz
-   **<float-delta-time>:** the time-interval between this sensor reading
    and the previous one, in seconds, e.g., 0.01.
-   **<float-sensor-reading>:** the sensor reading in whatever units
    you're measuring.


<a id="org58092cc"></a>

## Plotting the result

`plot_parallel.py` uses the `vertica_python` python module to read the data from
Vertica.  

It takes as arguments a list of column names and a table name, e.g., 

    python3 plot_parallel.py reading f001 f0016 f002 f01 f1 f5 lowpass_out

The "fnnn" column names are derived from 0.001, 0.0016, 0.002, 0.01,
.1 and .5 hertz.  On these plots, a 1 hertz signal would be 100 ticks
wide on the **x**-axis. 

Will create (separate) plots of the `reading`, `f001`, `f0016`,
`f002`, `f01`, `f1`, and  `f5` columns of the `lowpass_out` table
created by `lowpass_run.sql`.

It puts its output into a PNG file with a name constructed from the
table and column names, as well as running an interactive display
which can be manipulated.  Note that the vertical scales may differ.

Here is an example of the output:

![img](./lowpass_out_reading_f001_f0016_f002_f01_f1_f5_cols.png)

(X-axis measures "ticks" in the data-set &#x2014; ticks are uniformly
happening at 0.01 second in the lowpass dataset.)

Stacking the plots as `plot_parallel.py` does is useful when comparing
things with radically different ranges in the vertical axis.  For
things like the lowpass output, which cover the same range in the
vertical axis, you can also use `plot_overlay.py` to look at three
columns on one plot:

    python3 plot_overlay.py reading f001 f002 f01 lowpass_out

The results are seen here:

![img](./lowpass_out_overlay_reading_f001_f002_f01_cols.png)

This plot shows a problem with this implementation of a low-pass
filter: namely, as the critical frequency decreases the system has
more "inertia" responding to changes in the signal more slowly.  The
curve gets smoother, but part of that smoothness comes at a cost in
delay.


<a id="orgfb758bf"></a>

# High pass filter


<a id="orgb4c7aff"></a>

## Running the highpass filter on the low-pass data

The first argument in the `highpass` UDF is frequency in hertz.  In
this data-set, a one-hertz signal takes 100 units on the **x** axis.

    DROP TABLE IF EXISTS highpass_on_lpd_out CASCADE;
    CREATE TABLE highpass_on_lpd_out AS
    SELECT lowpass_data_with_deltas.row_n,
           reading,
           highpass(1, dt, reading) as f1,
           highpass(5, dt, reading) as f5,
           highpass(10, dt, reading) as f10,
           highpass(100, dt, reading) as f100
    FROM lowpass_data_with_deltas ORDER BY row_n; 

With the results shown here:

![img](./highpass_on_lpd_out_overlay_reading_f1_f5_f10_f100_cols.png)

Surprise!  The highpass signal is down around 0!  It doesn't follow
the offset of the signal.  Why?  The offset is **low-frequency data**
that gets filtered out.

If we set aside the raw input, we can see more detail in the highpass
output: 

![img](./highpass_on_lpd_out_overlay_f1_f5_f10_f100_cols.png)


<a id="org12f0c5c"></a>

## Running the highpass filter on the highpass data

The script `highpass_run.sql` runs the highpass filter on the highpass
dataset loaded by `highpass_load.sql`.

Plotted with 

    python3 plot_overlay.py reading f5 f100 f10000 highpass_out

(which ignores some of the columns) produces the following results:

![img](./highpass_out_overlay_reading_f5_f100_f10000_cols.png)


<a id="orgfd8a3a1"></a>

# Comparison of these filters to filtering with Fourier Transforms

Partly because of annoyance at the squiggles that remain in the
lowpass filter output, I decided to compare these functions to
filtering with Fourier transforms.  Basically, take Fourier Transform
of the signal, lop off the frequencies you're not interested in, then
take the inverse-Fourier Transform to see the filtered signal.

The file `fft.py` uses the `numpy.fft` module to take the Fast Fourier
Transform (FFT) of the entire dataset, then plots the result in
comparison to some of the results of the lowpass and highpass discrete
filters.

The following plot shows a comparison between filtering-via FFT and
filtering via discrete filters:

![img](./comparison_fft_to_discrete_filters.png)

The FFT software used reads the entire contents of the array into
memory, so it is limited in the size of the table it processes.

