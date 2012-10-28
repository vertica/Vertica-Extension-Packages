/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "LoadArgParsers.h"
#include <stdio.h>
#include <poll.h>
#include <sys/wait.h>
#include <errno.h>
#include "popen3.h"

using namespace Vertica;

//
// Superclass/mixin for User-Defined Load and Filter libraries which launch an
// external process.
//
class ProcessLaunchingPlugin {
private:    
    std::string cmd;
    std::vector<std::string> env;
    Popen3Proc child;
    bool running;
    
protected:
    ProcessLaunchingPlugin(std::string cmd, std::vector<std::string> env) : cmd(cmd), env(env) {}

    void setupProcess() {
        // Convert std::vector<std::string> to char *const envp[]
        std::vector<const char *>cStrArray(env.size()+1, NULL);
        for(int i = 0; i < env.size(); i++) {
            cStrArray[i] = env[i].c_str();
        }
        
        // Open child
        char *const argv[] = {"/bin/sh", "-c", const_cast<char *const>(cmd.c_str()), NULL};
        child = popen3(argv[0], argv, const_cast<char **>(&cStrArray[0]), O_NONBLOCK);

        // Validate the file handle; make sure we can read from this file
        if (child.pid < 0) {
            vt_report_error(0, "Error running child process (cmd = %s, code = %d): %s", cmd.c_str(), errno, strerror(errno));
        }
        running = true;
    }
    
    // Pumps data
    //   - from input to child process's stdin, and
    //   - from child process's stdout to output.
    StreamState pump(DataBuffer &input, InputState input_state, DataBuffer &output) {
        if (output.size == output.offset && input.size == input.offset && input_state != END_OF_FILE) {
            vt_report_error(0, "Can't read nor write, why poll?");
        }
        
        // Detect if the process has exited yet.
        // Has to be done before we check for data in the pipe;
        // if it's done before, we might miss that it has exited,
        // in which case we'll just loop again;
        // but if it's done after, we might miss the last packet
        // of data.
        checkProcessStatus(false);
        
        bool stdin_can_accept_data, stdout_has_data, stderr_has_data;
        if (ppoll3(child, &stdin_can_accept_data, &stdout_has_data, &stderr_has_data, 10)) {
            int err = errno;
            vt_report_error(0, "Error while doing poll() (%d): %s)", err, strerror(err));
        }
        
        // Buffer states
        bool input_buffer_empty = input.offset == input.size;
        bool output_buffer_full = output.offset == output.size;
        
        // If upstream is done, close our stdin handle
        // so that our wrapped process knows to stop.
        // Don't do this if we're currently processing data;
        // it's unnecessary and the received-data event
        // masks the EOF on some platforms with this implementation.
        if (input_state == END_OF_FILE && input_buffer_empty && child.stdin >= 0) {
            close(child.stdin);
            child.stdin = -1;
        }
        
        // Check stderr first since we treat it as fatal
        if (stderr_has_data) {
            // Use stderr for actual errors
            char errbuf[2048];
            ssize_t bytes_read = read(child.stderr, errbuf, 2047);
            if (bytes_read < 0) {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK) {
                    // Nothing to read, ignore
                } else {
                    vt_report_error(0, "Error while reading stderr of external process (%d): %s", err, strerror(err));
                }
            } else if (bytes_read > 0) {
                errbuf[bytes_read] = '\0';
                vt_report_error(0, "External process '%s' reported error: %s", cmd.c_str(), errbuf);
            } else if (bytes_read == 0) {
                // Child process closed it's stderr
                close(child.stderr);
                child.stderr = -1;
            }
        }
        
        if (stdin_can_accept_data && !input_buffer_empty) {
            ssize_t bytes_written = write(child.stdin, input.buf + input.offset, input.size - input.offset);
            if (bytes_written < 0) {
                int err = errno;
                vt_report_error(0, "Error while writing to stdin of external process (%d): %s", err, strerror(err));
            } else if (bytes_written >= 0) {
                input.offset += bytes_written;
            }
        }
        
        if (stdout_has_data && !output_buffer_full) {
            ssize_t bytes_read = read(child.stdout, output.buf + output.offset, output.size - output.offset);
            if (bytes_read < 0) {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK) {
                    // Nothing to read, ignore
                } else {
                    vt_report_error(0, "Error while reading stdout of external process (%d): %s", err, strerror(err));
                }
            } else if (bytes_read > 0) {
                output.offset += bytes_read;
            } else if (bytes_read == 0) {
                // Child process closed it's stdout
                close(child.stdout);
                child.stdout = -1;
            }
        }
        
        // Buffer states after reading and writing
        input_buffer_empty = input.offset == input.size;
        output_buffer_full = output.offset == output.size;
        
        // Return value
        if (child.stdin == -1 && child.stdout == -1) {
            return DONE;
        } else {
            if (input_state != END_OF_FILE && input_buffer_empty && child.stdin >= 0) {
                return INPUT_NEEDED;
            } else if (output_buffer_full && child.stdout >= 0) {
                return OUTPUT_NEEDED;
            } else {
                return KEEP_GOING;
            }
        }
    }
    
    void destroyProcess() {
        pclose3(child);
        checkProcessStatus(true);
    }
    
private:
    void checkProcessStatus(bool wait_for_term) {
        if (!running)
            return; // process terminated and termination status was already collected
        
        int status;
        pid_t waitpid_ret = waitpid(child.pid, &status, wait_for_term ? 0 : WNOHANG);
        if (waitpid_ret == -1) {
            int err = errno;
            vt_report_error(0, "Error retrieving the termination status of child (%d): %s", err, strerror(err));
        } else if (waitpid_ret == 0) {
            // Still running
        } else if (waitpid_ret == child.pid) {
            // Child terminated
            running = false;
            
            // Check termination status
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    // Success!
                } else {
                    vt_report_error(0, "Process exited with status %d", WEXITSTATUS(status));
                }
            } else if (WIFSIGNALED(status)) {
                vt_report_error(0, "Process killed by signal %d%s", WTERMSIG(status), WCOREDUMP(status) ? " (core dumped)" : "");
            } else {
                vt_report_error(0, "Process terminated with unexpected status - 0x%x\n", status);
            }
        } else {
            vt_report_error(0, "Internal error: waitpid returned %d (child pid is %d)", (int)waitpid_ret, child.pid);
        }
    }
};
