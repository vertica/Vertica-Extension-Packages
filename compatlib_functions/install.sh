# vsql to use: if VSQL env is set, use that otherwise use naked 'vsql'
if [ "X$VSQL_COMMAND" = "X" ]; then 
    VSQL_COMMAND="/opt/vertica/bin/vsql"
fi

# Root directory of connectby package: if $CONNETBYFN_HOME is set, use that otherwise use default
if [ "X$CONNETBYFN_HOME" = "X" ]; then 
    CONNETBYFN_HOME="."
fi

$VSQL_COMMAND -f $CONNETBYFN_HOME/src/ddl.sql

