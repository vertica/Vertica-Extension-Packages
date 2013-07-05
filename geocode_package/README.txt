-------------------------------
INTRODUCTION
-------------------------------

This package geocodes data using the USC WebGIS Geocoder free webservice.
Please see https://webgis.usc.edu/Services/Geocode/Default.aspx.

The code hardcodes a free license key obtained after registration with
limitations.  Please obtain your own key and make the code modifications
necessary.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

This package requires that you have the "curl" program and library 
installed on all machines in your cluster.  For example, on RedHat, as 
root, do

yum install curl

on all machines in your cluster.


You can then install this library by running the SQL statements in
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

In order to use the geocode webservice, this package is dependant upon the
curl and curl-devel libraries to build and run if it is not already installed
on your system.

This package was built using the libcurl version: 7.15.5

yum install curl
yum install curl-devel

To build:

$ make


-------------------------------
USAGE
-------------------------------

SELECT geocode(streetaddress,city,state,zip) 

INPUT: streetaddress,city,state,and zip = VARCHARS
RETURN: VARCHAR representing the latitude,longitude

Here is a sample run with the test data provided in the package.

[dbadmin@centos geocode_package]$ vsql -f examples/geocode.sql 
CREATE TABLE
    streetaddress    |   city    | state |  zip  |              geocode               
---------------------+-----------+-------+-------+------------------------------------
 8 Federal St        | Billerica | MA    | 01821 | 42.5455754814974,-71.2824066456301
 3000 Hanover Street | Palo Alto | CA    | 94304 | 37.4146176110341,-122.142834760344
                     | Chicago   | IL    |       | 41.8386549328395,-87.6866416802019
(3 rows)

DROP TABLE
 
-------------------------------
PERFORMANCE
-------------------------------

Extensive performance testing has not been completed yet but the performance
will be heavily be dependent upon the web service call.

Future improvements to the code:

1) More efficient web service response parsing
2) Build cache of addresses so that another geocode service call is not
invoked on the same address

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
