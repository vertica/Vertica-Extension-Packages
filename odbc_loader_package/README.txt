-------------------------------
INTRODUCTION
-------------------------------

ODBCLoader

This package contains a pair of User-Defined Load functions, ODBCSource() and ODBCLoader(), that can be used to load data directly from a remote database.

These Load functions connect to a remote database and load data directly from that database into Vertica.  Data is not converted into an intermediate file format; the data is fetched over the network via ODBC and each record is copied directly into Vertica as it is received.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install by running the SQL statements in
 ddl/install.sql 
or, to uninstall,
 ddl/uninstall.sql
Note that the SQL statements assume that you have copied this package to a 
node in your cluster and are running them from there.

Alternatively, assuming vsql is in your path, just do:

$ make install
$ make uninstall

As mentioned under 'BUILDING', to install the ODBC Loader you will need to have an ODBC driver manager installed on your cluster.

In order for the ODBC Loader to be useful, you will also need to install one or more ODBC drivers.  These must be installed on ALL COMPUTERS IN YOUR VERTICA CLUSTER!  The drivers are what enable the ODBC Loader to talk to other non-Vertica databases.  Each database product typically has its own driver; you'll need to install the driver per their documentation and packages.

-------------------------------
BUILDING
-------------------------------

To build this library from source code:

$ make

Note that this UDx is targeted against the Vertica 6.1 SDK or later.  If you are using Vertica 6.0, you can instead do

$ make CXXFLAGS="-DNO_LONG_OIDS"

The ODBC Loader depends on ODBC.  It is tested with the unixODBC driver manager.  unixODBC is available through all major Linux distribution's package managers; we recommend getting the supported version through your distribution.  It is also available from <http://www.unixodbc.org>.

unixODBC needs to be installed and configured IDENTICALLY on ALL COMPUTERS IN YOUR VERTICA CLUSTER in order to be able to use the ODBC Loader!

In order to compile the ODBC Loader, you will need the unixODBC development headers as well.  Most Linux distributions package these separately, in a package named something like "unixodbc-dev" or "unixodbc-devel".  You only need these installed on the machine you're using to compile the ODBC Loader.

Please make sure that you are compiling on your Vertica cluster (or at least on a machine that is running the same version and build of unixODBC as your Vertica cluster).  UnixODBC's ABI, particularly prior to version 2.3.0, is not 100% stable; you need to compile for the particular version that you are using.


-------------------------------
TESTING
-------------------------------

To test this library after building it:

$ make test

Unfortunately, this requires a lot of configuration to work.

1) Access to a mysql server is requried.  One way to do that is to install mariadb-server.  Follow the instructions and ensure that "mysql -u root" connects to mysql and can create a database.
2) Access to Vertica is required.  The community edition RPM can be downloaded for free and run on a single node (localhost).  The requirement is that /opt/vertica/bin/vsql connects to vertica and can create a table.
3) A mysql ODBC connector must be installed such as mysql-connector-odbc
4) An ODBC driver manager must be installed and configured on the vertica server such as unixODBC.  It needs a MySQL section in odbcinst.ini that lists the mysql odbc connector library.
5) /etc/odbc.ini should have a "Default" section to connect to mysql and a VerticaDCN section to connect to vertica.  Here is an example:
-------odbc.ini------------------------------------
[Default]
Driver       = /usr/lib64/libmyodbc5.so
Description  = UnixODBC
SERVER       = localhost
PORT         =
USER         = root
Password     =
Database     =
OPTION       =
SOCKET       =

[VerticaDSN]
Description = Vertica Data Source
Driver = /opt/vertica/lib64/libverticaodbc.so
Database = dbadmin
Servername = localhost
UID = dbadmin
Port = 5433
---------------------------------------------------
6) Optional: /etc/vertica.ini needs a "Driver" section for localized error messages
[Driver]
ErrorMessagesPath=/opt/vertica
LogLevel=4
LogPath=/tmp

-------------------------------
USAGE
-------------------------------

See examples/sample_usage.sql

In general, usage looks like:

COPY tbl WITH SOURCE ODBCSource() PARSER ODBCLoader(connect='DSN=some_odbc_dsn', query='select * from remote_table;');

This will cause Vertica to connect to the remote database identified by the given "connect" string and execute the given query.  It will then fetch the results of the query and load them into the table "tbl".

"tbl" must have the same number of columns as 'remote_table'.  The column types must also match up, or the ODBC driver for the remote database must be able to cast the column types to the Vertica types.  If necessary, you can always explicitly cast on the remote side by modifying the query, or on the local side with a Vertica COPY expression.

The 'query' argument to ODBCLoader can be any valid SQL query, or any other statement that the remote database will recognize and that will cause it to return a row-set.

The 'connect' argument to ODBCLoader can be any valid ODBC connect string.  It is common to configure /etc/odbc.ini and /etc/odbcinst.ini with all the necessary information, then simply reference the DSN listing in /etc/odbc.ini in each query.  For help configuring these files, or for more information on valid 'connect' strings, please see the documentation that came with the ODBC driver for the remote database product that you are connecting to, as the format of the string is specified by the driver.

WARNING:  The ODBC Loader CAN CRASH VERTICA if not used with THREAD-SAFE ODBC drivers!  Please see your driver's documentation to see if it is thread-safe by default or can be configured to operate in a thread-safe mode.

NOTE: There are essentially an unlimited number of permutations of remote database software, query types, data types, etc.  All of them behave a little differently.  If something doesn't work for you, patches welcome :-)

-------------------------------
DATABASE-SPECIFIC USAGE
-------------------------------

ORACLE:  All integers in Vertica are 64-bit integers.  Oracle doesn't support 64-bit integers; their ODBC driver can't even cast to them on request.  This code contains a quirk/workaround for Oracle that retrieves integers as C strings and re-parses them.  However, the quirk doesn't reliably detect Oracle database servers right now.  You can force Oracle with an obvious modification to the setQuirksMode() function in ODBCLoader.cpp.  If you know of a more-reliable way to detect Oracle, or a better workaround, patches welcome :-)

MYSQL:  The MySQL ODBC driver comes in both a thread-safe and thread-unsafe build and configuration.  The thread-unsafe version is KNOWN TO CRASH VERTICA if used in multiple COPY statements concurrently!  (Vertica is, after all, highly multithreaded.)  And distributions aren't consistently careful to package thread-safe defaults.  So if you're connecting to MySQL, be very careful to set up a thread-safe configuration.

VERTICA:  Why are you using this tool to connect one Vertica database to another Vertica database?  Don't do that!  Use Vertica's built-in IMPORT/EXPORT; it's dramatically faster.  Two big clusters talking through a single ODBC translation layer / pipe just isn't the best way to do it :-)

-------------------------------
PERFORMANCE
-------------------------------

The ODBC Loader has not been meaningfully performance-benchmarked to date.

It is constrained by how fast the remote database can execute the specified query.  It is also constrained by the network between the Vertica cluster and the remote database, as well as by any overhead in the ODBC driver being used.  For this reason, it's recommended to use a relatively simple query, to make sure the network between the two database systems is high-bandwidth and low-latency, and to read the documentation for the ODBC driver in use for the remote database to verify that it is properly configured for optimal performance.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt

