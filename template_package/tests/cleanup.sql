\! rm -f $TMPDIR/CompatLibFunctions.vpkg  

select version();

--clean up tables
drop table company;

--clean up library
DROP LIBRARY CompatLibFunctions CASCADE;
