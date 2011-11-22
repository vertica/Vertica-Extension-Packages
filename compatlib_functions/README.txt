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

See examples

-------------------------------
PERFORMANCE
-------------------------------

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt




