/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "LoadArgParsers.h"
#include <stdio.h>
#include <poll.h>
#include <sys/wait.h>

using namespace Vertica;

class ExternalSource : public UDSource {
private:
    FILE *handle;
    std::string cmd;

    virtual StreamState process(ServerInterface &srvInterface, DataBuffer &output) {
        pollfd pfd;
	pfd.fd = fileno(handle);
	pfd.events = POLLIN;
	pfd.revents = 0;
	int ret = poll(&pfd, 1, 1000);  // Timeout and try again after 1000ms

	if (ret == 0) {
	    return KEEP_GOING;
	} else if (ret < 0) {
	    vt_report_error(0, "Could not read from external process '%s'", cmd.c_str());
	}

        output.offset = fread(output.buf + output.offset, 1, output.size - output.offset, handle);
        return feof(handle) ? DONE : OUTPUT_NEEDED;
    }

public:
    ExternalSource(std::string cmd) : cmd(cmd) {}

    void setup(ServerInterface &srvInterface) {
        handle = popen(cmd.c_str(),"r");

        // Validate the file handle; make sure we can read from this file
        if (handle == NULL) {
            vt_report_error(0, "Error opening file [%s]", cmd.c_str());
        }
    }

    void destroy(ServerInterface &srvInterface) {
        int status = pclose(handle);

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                // Success!
            } else {
                vt_report_error(0, "Process exited with status %d", WEXITSTATUS(status));
            }
        } else if (WIFSIGNALED(status)) {
            vt_report_error(0, "Process killed by signal %d%s", WTERMSIG(status), WCOREDUMP(status) ? " (core dumped)" : "");
        } else {
            vt_report_error(0, "Process terminated with Unexpected status - 0x%x\n", status);
        }
    }
};

class ExternalSourceFactory : public SourceFactory {
public:

    virtual void plan(ServerInterface &srvInterface,
            NodeSpecifyingPlanContext &planCtxt) {

        /* Check parameters */
        std::vector<ArgEntry> argSpec;
        argSpec.push_back((ArgEntry){"cmd", true, VerticaType(VarcharOID, -1)});
        argSpec.push_back((ArgEntry){"nodes", false, VerticaType(VarcharOID, -1)});
        validateArgs("FileSource", argSpec, srvInterface.getParamReader());

        /* Populate planData */
        // Nothing to do here
        
        /* Munge nodes list */
        findExecutionNodes(srvInterface.getParamReader(), planCtxt, "ANY NODE");
    }


    virtual std::vector<UDSource*> prepareUDSources(ServerInterface &srvInterface,
            NodeSpecifyingPlanContext &planCtxt) {
        std::vector<UDSource*> retVal;
	retVal.push_back(vt_createFuncObj(srvInterface.allocator, ExternalSource,
					  srvInterface.getParamReader().getStringRef("cmd").str()));
        return retVal;
    }

    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes) {
        parameterTypes.addVarchar(65000, "cmd");
        parameterTypes.addVarchar(65000, "nodes");
    }
};
RegisterFactory(ExternalSourceFactory);
