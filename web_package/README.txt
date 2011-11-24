-------------------------------
INTRODUCTION
-------------------------------

The Web Analysis Package includes functions that are commonly used
for analysis of weblogs.

Apache Log Parser: Parse Apache Web Server log files

IIS / W3C Parser: Parse Microsoft IIS Web Server log files

URI decoder: Decodes the name=value pairs in URLs
http://en.wikipedia.org/wiki/Percent-encoding

Email Validator: 
-------------------------------
BUILDING
-------------------------------

To build:

$ make


-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

Assuming vsql is in your path, just do:

$ make install
$ make uninstall

Alternately, you can find the DDL that 'make install' uses in:
 src/ddl/install.sql 
and
 src/ddl/uninstall.sql

-------------------------------
USAGE
-------------------------------


*********************
* W3C Log Parser
*********************

The w3cLogParser function is used to parse logs stored using the W3C Extened 
Log File Format, such as those created by Microsoft IIS.

Reference:
http://www.microsoft.com/technet/prodtechnol/WindowsServer2003/Library/IIS/be22e074-72f8-46da-bb7e-e27877c85bca.mspx?mfr=true

To use this function, load your IIS log files into a staging table
with three columns: * an auto_increment field (to preserve ordering),
* A varchar field (to hold a line from the IIS log), and * a varchar
field (to hold the name of the file being read).

The w3cLogParser function is an analytic function that uses the window
clause to ensure that data is analyzed one file at a time.  Because
W3C log files can change their output dynamically at runtime, the
number and type of fields being output can change.

IIS will issue a #Fields: directive to indicate the change.  Without
the #Fields directive, the parser does not know how to map the fields
in the log to fields in the database.  To ensure that the parser
always knows what fields are being parsed, input to the function
should always be partitioned by filename (thus the reason for storing
the filename in the table holding the log data).

Please see the w3cLogParser.sql script for an example of how to stage
the data and call the function.

-------------------------------
PERFORMANCE
-------------------------------



-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
