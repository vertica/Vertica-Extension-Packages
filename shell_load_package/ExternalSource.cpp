/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "LoadArgParsers.h"
#include "ProcessLaunchingPlugin.cpp"

using namespace Vertica;

class ExternalSource : public UDSource, protected ProcessLaunchingPlugin {
    
    virtual StreamState process(ServerInterface &srvInterface,
                                DataBuffer &output)
    {
        DataBuffer input = {NULL, 0, 0};
        InputState input_state = END_OF_FILE;
        pump(input, input_state, output);
    }

public:
    ExternalSource(std::string cmd, std::vector<std::string> env) : ProcessLaunchingPlugin(cmd, env) {}

    void setup(ServerInterface &srvInterface) {
        setupProcess();
    }

    void destroy(ServerInterface &srvInterface) {
        destroyProcess();
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
        validateArgs("ExternalSource", argSpec, srvInterface.getParamReader());

        /* Populate planData */
        // Nothing to do here
        
        /* Munge nodes list */
        findExecutionNodes(srvInterface.getParamReader(), planCtxt, "ANY NODE");
    }


    virtual std::vector<UDSource*> prepareUDSources(ServerInterface &srvInterface,
            NodeSpecifyingPlanContext &planCtxt) {
        
        // I bet this could be written in 1/3 the lines... sorry :/
        
        std::string cmd = srvInterface.getParamReader().getStringRef("cmd").str();
        std::vector<std::string> env;
        env.push_back(std::string("CURRENT_NODE_NAME=") + srvInterface.getCurrentNodeName());
        
        std::vector<std::string> targetNodes = planCtxt.getTargetNodes();
        
        // Concatenate node names
        std::ostringstream targetNodeNamesStream;
        targetNodeNamesStream << "TARGET_NODE_NAMES=";
        for (int i = 0; i < targetNodes.size(); i++) {
            targetNodeNamesStream << targetNodes[i];
            if (i > 0) {
                targetNodeNamesStream << ",";
            }
        }
        env.push_back(targetNodeNamesStream.str());
        
        // Find index of current node
        int currentNodeIndex = -1;
        for (int i = 0; i < targetNodes.size(); i++) {
            if (targetNodes[i] == srvInterface.getCurrentNodeName()) {
                currentNodeIndex = i;
                break;
            }
        }
        if (currentNodeIndex == -1)
            vt_report_error(0, "Current node is not a target node?");
        
        std::ostringstream numTargetNodesStream;
        numTargetNodesStream << "NUM_TARGET_NODES=" << targetNodes.size();
        env.push_back(numTargetNodesStream.str());
        
        std::ostringstream currentNodeIndexStream;
        currentNodeIndexStream << "CURRENT_NODE_INDEX=" << currentNodeIndex;
        env.push_back(currentNodeIndexStream.str());
        
        std::vector<UDSource*> retVal;
        retVal.push_back(vt_createFuncObj(srvInterface.allocator, ExternalSource, cmd, env));
        return retVal;
    }

    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes) {
        parameterTypes.addVarchar(65000, "cmd");
        parameterTypes.addVarchar(65000, "nodes");
    }
};
RegisterFactory(ExternalSourceFactory);
