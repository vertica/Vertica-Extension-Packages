/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "LoadArgParsers.h"
#include "ProcessLaunchingPlugin.h"

using namespace Vertica;

class ExternalFilter : public UDFilter, protected ProcessLaunchingPlugin {
    
    virtual StreamState process(ServerInterface &srvInterface,
                                DataBuffer &input, InputState input_state,
                                DataBuffer &output)
    {
        pump(input, input_state, output);
    }

public:
    ExternalFilter(std::string cmd, std::vector<std::string> env) : ProcessLaunchingPlugin(cmd, env) {}

    void setup(ServerInterface &srvInterface) {
        setupProcess();
    }

    void destroy(ServerInterface &srvInterface) {
        destroyProcess();
    }
};

class ExternalFilterFactory : public FilterFactory {
public:

    virtual void plan(ServerInterface &srvInterface,
                      PlanContext &planCtxt) {

        /* Check parameters */
        std::vector<ArgEntry> argSpec;
        argSpec.push_back((ArgEntry){"cmd", true, VerticaType(VarcharOID, -1)});
        validateArgs("ExternalFilter", argSpec, srvInterface.getParamReader());

        /* Populate planData */
        // Nothing to do here
    }


    virtual UDFilter* prepare(ServerInterface &srvInterface,
                              PlanContext &planCtxt) {
        
        std::string cmd = srvInterface.getParamReader().getStringRef("cmd").str();
        std::vector<std::string> env;
        env.push_back(std::string("CURRENT_NODE_NAME=") + srvInterface.getCurrentNodeName());
        
        return vt_createFuncObj(srvInterface.allocator, ExternalFilter, cmd, env);
    }

    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes) {
        parameterTypes.addVarchar(65000, "cmd");
    }
};
RegisterFactory(ExternalFilterFactory);
