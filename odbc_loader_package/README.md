## INTRODUCTION
This package contains two User-Defined Load functions, **ODBCSource()** and **ODBCLoader()**, that can be used to:
- load data directly from a remote database
- run queries against remote databases from Vertica (for example to join local Vertica-managed tables with MySQL, PostgreSQL, Oracle, etc.)

Data retrieved from external databases is neither converted into an intermediate file formats nor landed to disk; the data is fetched over the network via ODBC and copied directly into Vertica as it is received. When used with Vertica versions 10.1 and above *predicate pushdown* and *column filtering* is applied to the remote database data extraction process. 

## INSTALLING / UNINSTALLING
### Preprequisites
In order to install the ODBCLoader package you need to install on **all nodes of your Vertica cluster**:
- an ODBC Driver Manager (it has been tested with unixODBC)
- the ODBC Drivers to interface the remote databases
- configure the ODBC configuration files (see the examples here below)

In order to compile the ODBCLoader you also have to setup a development environment as defined in the standard documentation ([Setting Up a Development Environment](https://www.vertica.com/docs/10.1.x/HTML/Content/Authoring/ExtendingVertica/UDx/DevEnvironment.htm)).

### Building and installing the library
This operation has to be executed as ``dbadmin`` from **one of the node of the cluster** (Vertica will *propagate* the libraries across the cluster):
```
$ make
$ make install
```

### Uninstalling the library
To remove the ODBCLoader library from your cluster: 
```
$ make uninstall
```

## Usage

The ODBCLoader can be used to load data from external databases or to query non-Vertica databases through external tables.

### Data loading
The general syntax to load data from external databases via ODBCLoader is:
```
COPY myschema.myverticatable 
    WITH SOURCE ODBCSource() PARSER ODBCLoader(
        connect='DSN=some_odbc_dsn;<other connection parameters>',  
        query='select * from remote_table',
        [rowset=<number>]
    )
;
```
where ``rowset`` is an optional parameter to define the number of rows fetched from the remote database in each SQLFetch() call (default = 100). Increasing this parameter can improve the performance but will also increase memory usage.
 
This will cause Vertica to connect to the remote database identified by the given "connect" string and execute the given query.  It will then fetch the results of the query and load them into the table ``myschema.myverticatable``.

``myverticatable`` must have the same number of columns as ``remote_table``.  The column types must also match up, or the ODBC driver for the remote database must be able to cast the column types to the Vertica types.  If necessary, you can always explicitly cast on the remote side by modifying the query, or on the local side with a Vertica COPY expression.

The ``query`` argument to ODBCLoader can be any valid SQL query, or any other statement that the remote database will recognize and that will cause it to return a rowset.

The ``connect`` argument to ODBCLoader can be any valid ODBC connect string.  It is common to configure /etc/odbc.ini and /etc/odbcinst.ini with all the necessary information, then simply reference the DSN listing in /etc/odbc.ini in each query.  For help configuring these files, or for more information on valid 'connect' strings, please see the documentation that came with the ODBC driver for the remote database product that you are connecting to, as the format of the string is specified by the driver.

### Federated queries
As we said we can use the ODBCLoader to run *federated queries* against other databases (for example to join Vertica tables with MySQL tables) taking advantage of both *predicate pushdown* and *column pruning* in order to move from the external database to Vertica only the data really needed.

To use this feature we Use Vertica External Tables as a gateway. Let's use an example to illustrate the process. External Tables, differently from Vertica-managed tables, leave the data *outside Vertica* In the following example we define an External Table in Vertica (public.epeople) retrieving data from the ODBC Data Source "pmf" through the query ``SELECT * FROM public.people``:

```sql
CREATE  EXTERNAL  TABLE  public.epeople(
    id INTEGER,
    name VARCHAR(20)
) AS  COPY  WITH
    SOURCE  ODBCSource()
    PARSER  ODBCLoader(
        connect='DSN=pmf',
        query='SELECT * FROM public.people'
) ;
```

When you define the External Table **nothing** is retrieved from the external database; we just save - in Vertica - the information needed to extract the data from the external source.

Now, if we run, in Vertica, a query like this:
```sql
SELECT * FROM public.epeople WHERE id > 100;
```
The ODBCLoader will rewrite the original query defined in the previous External Table definition as follow:
```sql
SELECT id, name FROM public.people WHERE id > 100;
```
As you can see this query will *pushdown* the predicate to the external database.

The other important feature to limit the amount of data being moved from the external database to Vertica is **columns pruning**.  This means we extract from the external database only the columns needed to run the query in Vertica. As we have to retrieve all columns defined in the External Table, the ones not needed in the Vertica query will be replaced by NULL. So, for example, if we run, in Vertica:
```sql
SELECT id FROM public.epeople WHERE id > 100;
```
the following query will be executed against the external database:
```sql
SELECT id, NULL FROM public.people WHERE id > 100;
```

### Optional configuration switches
The ODBCLoader accept, in the External Table definition, the following, optional, configuration switches.

Amount of rows retrieved during each SQLFetch() iteration from the external database. The **default value for this parameter is 100** and you can alter it in the ``CREATE EXTERNAL TABLE`` statement by defining a different ``rowset``. For example:

```sql
CREATE  EXTERNAL  TABLE  public.epeople(
    id INTEGER,
    name VARCHAR(20)
) AS  COPY  WITH
    SOURCE  ODBCSource()
    PARSER  ODBCLoader(
        connect='DSN=pmf',
        query='SELECT * FROM public.people',
        rowset = 500
) ;
```
Please consider that increasing this parameter will also increase the memory consumption of the ODBCLoader.

You can **switch predicate pushdown on/off** with the optional boolean parameter  ``src_rfilter``. The default value is true (meaning we perform predicate pushdown). You can set  ``src_rfilter``  either in the ``CREATE EXTERNAL TABLE`` statement or using a **SESSION PARAMETER** as follows:
```sql
ALTER SESSION SET UDPARAMETER FOR ODBCLoaderLib src_rfilter = false ;
```

You can **switch columns filtering on/off** with the optional boolean parameter  ``src_cfilter``. The default value is true (meaning we perform predicate pushdown). You can set ``src_cfilter``  either in the ``CREATE EXTERNAL TABLE`` statement or using a **SESSION PARAMETER** as follows:
```sql
ALTER SESSION SET UDPARAMETER FOR ODBCLoaderLib src_cfilter = false ;
```

You can overwrite the query defined in the original ``CREATE EXTERNAL TABLE`` statement using the session parameter ``override_query`` as follows:
```sql
ALTER SESSION SET UDPARAMETER FOR ODBCLoaderLib override_query = '
    SELECT * FROM people WHERE id % 2
'
```
The default length for session parameters is 2kB so, if your override_query is longer you might need to increase the session parameter max length before setting ``override_query``. For example:
```sql
ALTER SESSION SET MaxSessionUDParameterSize = 16384 ;
ALTER SESSION SET UDPARAMETER FOR ODBCLoaderLib override_query = '
     SELECT 00 AS id, name, gender,bdate FROM mmf.mypeople UNION ALL 
     SELECT 01 AS id, name, gender,bdate FROM mmf.mypeople UNION ALL 
     SELECT 02 AS id, name, gender,bdate FROM mmf.mypeople UNION ALL
     SELECT 03 AS id, name, gender,bdate FROM mmf.mypeople UNION ALL
     SELECT 04 AS id, name, gender,bdate FROM mmf.mypeople UNION ALL
... long query here ...
'
```
**Please note**: session parameters are - of course -  *session-scoped*.  Have a look to the standard Vertica documentation to learn how to set/clear/check [User-Defined Session Parameters](https://www.vertica.com/docs/10.1.x/HTML/Content/Authoring/ExtendingVertica/UDx/Parameters/UDSessionParameters.htm).

### Pushed-down predicates conversion
When pushing predicates down to the external database, ODBCLoader performs the following changes:
1. Vertica specific data type casting ``::<data type>`` will be removed
2. ``~~`` will be converted to ``LIKE``
3. ``ANY(ARRAY)`` will be converted to ``IN()``. For example ``ID = ANY(ARRAY[1,2,3])`` will be converted to ``ID IN(1,2,3)``
 
## How to debug the ODBC layer
The simpler way to check how the ODBCLoader rewrite the query sent to the external database is to enable ODBC traces in odbcinst.ini. For example:
```
[ODBC]
Trace=on
Tracefile=/tmp/uodbc.trc
```
And then grep the SQL from the trace file:
```
$ tail -f /tmp/uodbc.trc | grep 'SQL = '
```
Please remember to switch ODBC traces off at the end of your debug session because they will slowdown everything and create huge log files...

## Sample ODBC Configurations
The following two configuration files ```odbc.ini``` and ```odbcinst.ini``` have been used to define two data sources: **pmf** to connect to PostgreSQL and **mmf** to connect to MySQL:
```
$ cat /etc/odbc.ini
[ODBC Data Sources]
PSQLODBC  = PostgreSQL ODBC
MYODBC  = MySQL ODBC

[pmf]
Description  = PostgreSQL mftest2
Driver = PSQLODBC
Trace  = No
TraceFile  = sql.log
Database = pmf
Servername = mftest2
UserName =
Password =
Port = 5432
SSLmode  = allow
ReadOnly = 0
Protocol = 7.4-1
FakeOidIndex = 0
ShowOidColumn  = 0
RowVersioning  = 0
ShowSystemTables = 0
ConnSettings =
Fetch  = 1000
Socket = 4096
UnknownSizes = 0
MaxVarcharSize = 1024
MaxLongVarcharSize = 8190
Debug  = 0
CommLog  = 0
Optimizer  = 0
Ksqo = 0
UseDeclareFetch  = 0
TextAsLongVarchar  = 1
UnknownsAsLongVarchar  = 0
BoolsAsChar  = 1
Parse  = 0
CancelAsFreeStmt = 0
ExtraSysTablePrefixes  = dd_
LFConversion = 0
UpdatableCursors = 0
DisallowPremature  = 0
TrueIsMinus1 = 0
BI = 0
ByteaAsLongVarBinary = 0
LowerCaseIdentifier  = 0
GssAuthUseGSS  = 0
XaOpt  = 1
UseServerSidePrepare = 0

[mmf]
Description  = MySQL mftest2
Driver = MYODBC
SERVER = mftest2
PORT = 3306
SQL-MODE = 'ANSI_QUOTES'

$ cat /etc/odbcinst.ini
[ODBC]
Trace=off
Tracefile=/tmp/uodbc.trc

[PSQLODBC]
Description=PostgreSQL ODBC Driver
Driver64=/usr/lib64/psqlodbcw.so
UsageCount=1

[MYODBC]
Driver=/usr/lib64/libmyodbc8w.so
UsageCount=1

[MySQL ODBC 8.0 ANSI Driver]
Driver=/usr/lib64/libmyodbc8a.so
UsageCount=1
```



