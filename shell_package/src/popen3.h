/**
 * Implementation of the "popen3" function.
 * Like "popen" but provides pipes for all three of (stdin, stdout, stderr)
 */

#ifndef POPEN3_H
#define POPEN3_H

#include <sys/types.h>

/**
 * Popen3Proc
 * 
 * A struct representing a process that has been popen3'ed.
 * Its stdin, stdout, and stderr are available for reading/writing.
 */
struct Popen3Proc {
  /// Process ID of the child process
  pid_t pid;

  /// File descriptors for the various pipes used by popen3
  int stdin;
  int stdout;
  int stderr;
};

/**
 * popen3
 * 
 * Take a command to execute (as an argument to "/bin/sh -c").
 * Fork and execute that command as a side process.
 * Connect to its stdin, stout, and stderr; return file descriptors
 * referencing all three inputs.
 * This method is similar to the standard popen() API call,
 * except that it always provides handles to all three fd types,
 * rather than just stdout or just stdin.
 */
Popen3Proc popen3(const char *filename, 
                  char *const argv[],
                  char *const envp[],
                  int flags);

/**
 * pclose3
 *
 * The counterpart to popen3.  Closes a side process that was opened;
 * cleans up any hanging pipes / file descriptors.
 */
void pclose3(Popen3Proc &proc);

/**
 * ppoll3
 * 
 * poll()'s stdin, stdout and stderr.
 */
int ppoll3(Popen3Proc &proc,
           bool *stdin_writable,
           bool *stdout_readable,
           bool *stderr_readable,
           int timeout);

#endif
