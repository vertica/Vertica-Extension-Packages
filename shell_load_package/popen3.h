

/**
 * Implementation of the "popen3" function.
 * Like "popen" but provides pipes for all three of (stdin, stdout, stderr)
 */

#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

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
 * pclose3
 *
 * The counterpart to popen3.  Closes a side process that was opened;
 * cleans up any hanging pipes / file descriptors.
 */
static void pclose3(Popen3Proc &proc) {
  if (proc.stdin) { close(proc.stdin); proc.stdin = -1; }
  if (proc.stdout) { close(proc.stdout); proc.stdout = -1; }
  if (proc.stderr) { close(proc.stderr); proc.stderr = -1; }
}

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
static Popen3Proc popen3(const char *cmd, int flags = 0) {
  // Build our return object
  Popen3Proc proc;

  // argv string to (eventually) be passed to /bin/sh
  const char *argv[4];
  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = cmd;
  argv[3] = NULL;

  // envp string to be passed to /bin/sh
  // For now, no environment.
  char *envp[1];
  envp[0] = NULL;

  // Make some pipes
  int stdin[2], stdout[2], stderr[2];

  pipe(stdin);
  pipe(stdout);
  pipe(stderr);

  // Fork ourselves.
  // Have the child hook up the std{in,out,err} pipes and exec cmd.
  pid_t result = vfork();
  if (result == 0) {
    close(stdin[1]);
    close(stdout[0]);
    close(stderr[0]);

    close(0);
    if (dup(stdin[0]) < 0) { _exit(-1); }
    close(1);
    if (dup(stdout[1]) < 0) { _exit(-1); }
    close(2);
    if (dup(stderr[1]) < 0) { _exit(-1); }

    if (execve("/bin/sh", const_cast<char* const*>(argv), envp)) {
      _exit(-1);
    }
  }

  // Close remote process's end of our pipes
  close(stdin[0]);
  close(stdout[1]);
  close(stderr[1]);

  // Store the relevant handles and return the proc object that we've built
  proc.pid = result;
  proc.stdin = stdin[1];
  proc.stdout = stdout[0];
  proc.stderr = stderr[0];

  // Apply flags only to our end of the fd's
  // The target forked program presumably expects a plain-vanilla fifo.
  fcntl(proc.stdin, F_SETFL, fcntl(proc.stdin, F_GETFL, 0) | flags);
  fcntl(proc.stdout, F_SETFL, fcntl(proc.stdout, F_GETFL, 0) | flags);
  fcntl(proc.stderr, F_SETFL, fcntl(proc.stderr, F_GETFL, 0) | flags);

  // Also make sure our pipes don't survive future forkage.
  // Otherwise, bad things can happen if popen3() is called multiple times.
  fcntl(proc.stdin, F_SETFD, fcntl(proc.stdin, F_GETFD, 0) | FD_CLOEXEC);
  fcntl(proc.stdout, F_SETFD, fcntl(proc.stdout, F_GETFD, 0) | FD_CLOEXEC);
  fcntl(proc.stderr, F_SETFD, fcntl(proc.stderr, F_GETFD, 0) | FD_CLOEXEC);

  return proc;
}
