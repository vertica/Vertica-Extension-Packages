# vsql to use: if VSQL env is set, use that otherwise use naked 'vsql'
if [ "X$VSQL_COMMAND" = "X" ]; then 
    VSQL_COMMAND="/opt/vertica/bin/vsql"
fi

# Root directory of package: if $PACKAGE_HOME is set, use that otherwise use default
if [ "X$PACKAGE_HOME" = "X" ]; then 
    PACKAGE_HOME="."
fi

$VSQL_COMMAND -f $PACKAGE_HOME/src/ddl.sql

