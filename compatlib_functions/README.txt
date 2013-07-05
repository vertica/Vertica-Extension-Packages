-------------------------------
INTRODUCTION
-------------------------------

Compatibility Functions

This package contains a set of functions that are found in other
databases (such as Oracle) but not natively in the Vertica Analytic
Database.

These implementations might be incomplete. Please help by updating them!

CONNECT_BY (http://psoug.org/reference/connectby.html)
TRANSPOSE  (http://www.orafaq.com/node/1871)

GROUP_GENERATOR - use to build ROLLUP, CUBE, GROUPING_SETS, MULTIPLE DISTINCT AGGREGATES in the NEW EE.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install this package by running the SQL commands in:
 src/ddl/install.sql 
or, to uninstall,
 src/ddl/uninstall.sql
Note that the SQL statements assume that you have copied this package to a 
node in	your cluster and are running them from there.

Alternatively, assuming vsql is in your path, just do:

$ make install
$ make uninstall

-------------------------------
BUILDING
-------------------------------

To build from source:

$ make

-------------------------------
USAGE
-------------------------------

See examples (in the examples/ directory)

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt




