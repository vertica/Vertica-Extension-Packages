-------------------------------
INTRODUCTION
-------------------------------

The Shell extension package provides a mechanism to execute shell
commands from within Vertica and retrieve the results as a collection
of rows.  Be aware: executing shell commands (as the same user as the
vertica process!) is a loaded gun and should be used with care.  The
Shell package can be used for collecting diagnostics, launching
database-external processing, or database administration.  By
correctly formulating the query that invokes the Shell function,
processes can be spawned on any subset of nodes in the cluster.

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install the package by running the SQL commands in
 src/ddl/install.sql 
or, to uninstall,
 src/ddl/uninstall.sql
Note that the SQL statements assume that you have copied this package to a
node in your cluster and are running them from there.

Alternatively, assuming vsql is in your path, just do:

$ make install
$ make uninstall

Make sure the user running the Vertica process is able to sudo to user 'nobody'
without password and without having a tty. The following /etc/sudoers entry
should work:

dbadmin ALL=(nobody) NOPASSWD: ALL
Defaults:dbadmin !requiretty

Alternatively, add -DNO_SUDO to CXXFLAGS in the Makefile to have the
commands run as the Vertica process, and rebuild from source. Note that
even though running as 'nobody' is not secure (one user could harm or
inspect another user's process), at least it is a bit safer in that an
accidental "rm -rf" will not delete your database.

-------------------------------
BUILDING
-------------------------------

To build from source code:

$ make


-------------------------------
USAGE
-------------------------------

shell_execute('id','command')

Arguments:
id      - typically the node_name, is passed directly through to the output
command - command to execute.

Output columns:
id      - passed in
command - passed in
text    - lines of output (1 row per line), truncated to 64000 characters

The actual execution is accomplished by forking and running: bash -c <command>
Because the command is actually interpreted by the shell, pipes,
redirects, looping and other shell functionality is available.  One
row per line of output is produced.

Example:
select shell_execute(local_node_name(),'date') over ();

Distributed execution is accomplished by sourcing the rows fed to
shell_execute from a segmented projection.  The installer builds an
appropriate projection and loads it with data: exactly 1 row per node.
By partitioning the shell_execute transform function on the
segmentation column, Vertica will generate a plan where an instance of
shell_execute runs exactly once on each node in the cluster.

Example:
select shell_execute(node_name, 'tail -10 vertica.log') over (partition by segval) from onallnodes order by id;

See examples/shell.sql for a number of other example uses.
See examples/diagnostics.sql for a script to collect diagnostics through vsql + shell extension
See examples/copy-cmd-writing.sql for a script to stage & load files from an ftp server

Notes:
* A badly formed shell command does not error the query; the text
  value describes the error.
* If shell command produces no output, one row with a NULL text value
  is output.


int_sequence(start,end)

start - integer to start at (inclusive)
end   - integer to end at (inclusive)

Generates integers, one per row in the given bounds.  Included here
because it is used to generate the projections for distributed
execution.

If end > start, counts up.
If start > end, counts down.

-------------------------------
PERFORMANCE
-------------------------------

vfork() has some performance implications.  Needs some performance
measurement on very large, very active databases.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
