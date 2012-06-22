/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "LoadArgParsers.h"
#include "popen3.h"
#include <poll.h>
#include <errno.h>
#include <iostream>

using namespace Vertica;

class ExternalFilter : public UDFilter {
private:

  Popen3Proc handle;
  std::string cmd;
  bool has_exited;

  const static int GOT_NOTHING = 0;
  const static int GOT_STDIN = 1;
  const static int GOT_STDOUT = 2;
  const static int GOT_STDERR = 4;

  int waitForData() {
    pollfd pollfds[3];
    pollfds[1].fd = handle.stdout;
    pollfds[1].events = POLLIN | POLLHUP;
    pollfds[1].revents = 0;
    pollfds[0].fd = handle.stderr;
    pollfds[0].events = POLLIN | POLLHUP;
    pollfds[0].revents = 0;
    pollfds[2].fd = handle.stdin;
    pollfds[2].events = POLLOUT | POLLHUP;
    pollfds[2].revents = 0;

    // Don't poll stdin if we've already closed it
    int num_changed = poll(&pollfds[0], (handle.stdin >= 0) ? 3 :2, 10);
    int got = GOT_NOTHING;

    if (num_changed != 0) {
      if (pollfds[1].revents) got |= GOT_STDOUT;
      if (pollfds[0].revents) got |= GOT_STDERR;
      if (pollfds[2].revents) got |= GOT_STDIN;
    }

    return got;
  }
    

    virtual StreamState process(ServerInterface &srvInterface,
				DataBuffer &input, InputState input_state,
				DataBuffer &output)
   {
     // Trying to read or write 0 data is a bad idea
     // (0 == EOF in UNIX-land.)
     if (output.size == output.offset) {
       return OUTPUT_NEEDED;
     }
     if (input.size == input.offset && input_state != END_OF_FILE) {
       return INPUT_NEEDED;
     }


     // Detect if the process has exited yet.
     // Has to be done before we check for data in the pipe;
     // if it's done before, we might miss that it has exited,
     // in which case we'll just loop again;
     // but if it's done after, we might miss the last packet
     // of data.
     {
       int status = 0;
       int ret = waitpid(handle.pid, &status, WNOHANG);
       has_exited = (ret == handle.pid && WIFEXITED(status));
     }
     

     // Use poll() to wait until we have data to work with.
     int got = waitForData();

     // If upstream is done, close our stdin handle
     // so that our wrapped process knows to stop.
     // Don't do this if we're currently processing data;
     // it's unnecessary and the received-data event
     // masks the EOF on some platforms with this implementation.
     if (input_state == END_OF_FILE
	 && input.offset == input.size
	 && handle.stdin >= 0) {
       close(handle.stdin);
       handle.stdin = -1;
     }

     // Check stderr first since we treat it as fatal
     if (got & GOT_STDERR) {
       // Use stderr for actual errors
       char errbuf[2048];
       ssize_t bytes_read = read(handle.stderr, errbuf, 2047);
       int err = errno;

       if (bytes_read < 0 && (err == EAGAIN || err == EWOULDBLOCK)) {
	 bytes_read = 0;
       }
       if (bytes_read > 0) {
	 errbuf[bytes_read] = '\0';
	 vt_report_error(0, "Error reported from command-line application '%s': [%s] (%d bytes)", cmd.c_str(), errbuf, bytes_read);
       } else if (bytes_read < 0) {
	 vt_report_error(0, "Error occurred retrieving an error message from command-line application '%s' (%d)", cmd.c_str(), err);
       }
     }
     
     if (got & GOT_STDIN && input.offset != input.size) {
       ssize_t bytes_written = write(handle.stdin, input.buf + input.offset, input.size - input.offset);
       if (bytes_written >= 0) {
	 input.offset += bytes_written;
       } else {
	 vt_report_error(0, "Unknown error writing data to external process: %zd (%d)", bytes_written, errno);
       }
     }
    
     if (got & GOT_STDOUT) {
       ssize_t bytes_read = read(handle.stdout, output.buf + output.offset, output.size - output.offset);
       int err = errno;
       
       if (bytes_read < 0 && (err == EAGAIN || err == EWOULDBLOCK)) {
	 bytes_read = 0;
       }
       if (bytes_read == 0 && (err != EAGAIN && err != EWOULDBLOCK)) {
	 return DONE;  // The remote process finished and closed
       } else if (bytes_read >= 0) {
	 output.offset += bytes_read;
       } else {
	 vt_report_error(0, "Unknown error reading data from external process: %z (%d)", bytes_read, err);
       }
     }

     if (has_exited && !got) {
       return DONE;
     }

     return (got & GOT_STDIN) ? INPUT_NEEDED : (got & GOT_STDOUT ? OUTPUT_NEEDED : KEEP_GOING);
    }

public:
    ExternalFilter(std::string cmd) : cmd(cmd) {}

    void setup(ServerInterface &srvInterface) {
      handle = popen3(cmd.c_str(), O_NONBLOCK);
      has_exited = false;

      // Validate the file handle; make sure we can read from this file
      if (handle.pid == -1) {
	vt_report_error(0, "Error running side process [%s]", cmd.c_str());
      }     
    }

    void destroy(ServerInterface &srvInterface) {
        pclose3(handle);
	has_exited = true;
    }
};

class ExternalFilterFactory : public FilterFactory {
public:

    virtual void plan(ServerInterface &srvInterface,
		      PlanContext &planCtxt) {

        /* Check parameters */
        std::vector<ArgEntry> argSpec;
        argSpec.push_back((ArgEntry){"cmd", true, VerticaType(VarcharOID, -1)});
        validateArgs("FileSource", argSpec, srvInterface.getParamReader());

        /* Populate planData */
        // Nothing to do here
    }


    virtual UDFilter* prepare(ServerInterface &srvInterface,
			      PlanContext &planCtxt) {
	return vt_createFuncObj(srvInterface.allocator, ExternalFilter,
				srvInterface.getParamReader().getStringRef("cmd").str());
    }

    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes) {
        parameterTypes.addVarchar(65000, "cmd");
    }
};
RegisterFactory(ExternalFilterFactory);
