-- do setup for this package (i.e., install the package and create the functions)

\! BOOST_HOME=$SOURCE/../third-party/boost/boost_1_38_0 ../../tools/make_package -d .. -o $TMPDIR/CompatLibFunctions.vpkg -s $TARGET/VerticaSDK

\! ../../tools/install_package -p $TMPDIR/CompatLibFunctions.vpkg 

select version();

create table company (id int, supervisor_id int, name varchar(20));

