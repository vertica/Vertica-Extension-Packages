-------------------------------
INTRODUCTION
-------------------------------

This is a template designed to assist creating packages of extensions
to the Vertica Analytic Database.

You should fill out this section with a high level summary of the
package and the functionality it provides.

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

This section should contain instructions on usage of the functions
contained within the package. All functions should be accompanied with
an example of use in the examples directory.

-------------------------------
PERFORMANCE
-------------------------------

This section should contain any performance metrics you may have
measured for your package. Please include what hardware and data size
your measurements were performed on.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
