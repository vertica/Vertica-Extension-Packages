
***************************************
* Vertica Analytic Database
*
* AvroParser - User-Defined Parser to import Avro data files into Vertica
*
* Ramachandra CN & Purnam Jantrania 
***************************************

This package is a user defined parser that allows the user to import data from 
a file in the Apache Avro format into a Vertica table.

Installing this parser requires Avro and Boost libraries, which are included in
the 'tparty' directory.  Building the parser also builds those libraries.


***************************************
* Installation steps
***************************************

To install the pre-compiled binary:

- Copy `lib/AvroParser.so' to a machine in your Vertica cluster

- Connect to that machine with vsql and run the following SQL statements:
CREATE LIBRARY AvroParserLib AS '/path/to/AvroParser.so';
CREATE PARSER AvroParser AS LANGUAGE 'C++' NAME 'AvroParserFactory' LIBRARY AvroParserLib;

(That's it!)


To install from source code:

- Change to the downloaded directory (if you aren't there already ;) )
    cd Avro

- Change the SDK symbol in the Makefile to point to the Vertica SDK (by default it is /opt/vertica/sdk)

- Build and install the third-party add-ons and the AvroParser library
    make build 

- Start vertica

- Set the TMPDIR variable to the current directory (needed for the example/tests)
    export TMPDIR=`pwd`

- Install the AvroParser.so library as a parser in Vertica
    make install

- Run the tests
    make test

***************************************
* Uninstallation steps
***************************************
To uninstall the AvroParser libraray (and drop the parser from Vertica), run
  make uninstall

***************************************
* Example usage
***************************************
From the 'Avro' directory, run
   vsql -f examples/example.sql # example.sql includes a setup and usage example
