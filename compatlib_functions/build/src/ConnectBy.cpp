#include "Vertica.h"
#include <sstream>
#include <map>

using namespace Vertica;
using namespace std;


const int WIDTH = 2000;

class ConnectBy : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
    	map<vint, vint> parent;
    	map<vint, string> label;
    	map<vint, string> separator;


    	if (input_reader.getNumCols() != 4)
            vt_report_error(0, "Function only accepts 4 argument, but %zu provided", input_reader.getNumCols());

        do {
            parent[input_reader.getIntRef(1)] = input_reader.getIntRef(0);
            label[input_reader.getIntRef(1)] = input_reader.getStringRef(2).str();
            separator[input_reader.getIntRef(1)] = input_reader.getStringRef(3).str();

            std::string inString = input_reader.getStringRef(2).str();

            //srvInterface.log(" adding %s ", inString.c_str());


        } while (input_reader.next());
        //srvInterface.log("1");
        // exit(0);

        map<vint, string> cache;
        map<vint, vint> depth;
        map<vint, string>::iterator p;

        for (p = label.begin(); p != label.end(); ++p) {
        	if (cache.count(p->first) == 0) {

        		string output = p->second;
        		vint current_depth = 0;
        		vint current = parent[p->first];
        		while (current != 0 ) {

        	        srvInterface.log("working on %lld", current);

					if (cache.count(current) > 0 ) {
        				// Found the parent's path in the cache
        				output = cache[current] + separator[current] + output;
        				current_depth = depth[current] + 1ull;
        				break;
        			} else {
        				// prepend parent label + separator
        				output = label[current] + separator[current] + output;
        				current = parent[current];
        				current_depth = current_depth + 1ull;
        			}
        		}

        		cache[p->first] = output;
        		depth[p->first] = current_depth;
        	}

            srvInterface.log("attempting to output %lld", p->first);

        	output_writer.setInt(0,p->first);
        	output_writer.setInt(1,depth[p->first]);
			VString &word = output_writer.getStringRef(2);
			word.copy(cache[p->first]);
			output_writer.next();
        }


    }


};


class ConnectByFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 5 inputs (parent ID, child ID, label, separator,
	// and return a row with 2 strings (id, path)
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addInt();
        argTypes.addInt();
        argTypes.addVarchar();
        argTypes.addVarchar();

        returnType.addInt();
        returnType.addInt();
        returnType.addVarchar();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 1 argument
        if (input_types.getColumnCount() != 4)
            vt_report_error(0, "Function only accepts 3 arguments, but %zu provided", input_types.getColumnCount());


        // Our output size will never be more than the input size
        output_types.addInt("identifier");
        output_types.addInt("depth");
        output_types.addVarchar(WIDTH, "path");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, ConnectBy); }

};

RegisterFactory(ConnectByFactory);


