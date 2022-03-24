#include "Vertica.h"
#include <string>
#include <map>

using namespace Vertica;
using namespace std;


const int WIDTH = 65000;

class ConnectBy : public TransformFunction
{
    BaseDataOID argTypeOID;  // data type OID of parent ID and child ID

    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes)
    {
        // Get data type OID of parent ID and child ID
        const VerticaType argType = argTypes.getColumnType(0);
        argTypeOID = argType.getTypeOid();
    }

    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
    	map<string, string> parent;
    	map<string, string> label;
    	map<string, string> separator;

    	if (input_reader.getNumCols() != 4)
            vt_report_error(0, "Function only accepts 4 argument, but %zu provided", input_reader.getNumCols());

        string childValue = "";

        do {
            childValue = getArgValueAsString(input_reader, 1);
            if (childValue == "")
                continue;

            parent[childValue] = getArgValueAsString(input_reader, 0);
            label[childValue] = input_reader.getStringRef(2).str();
            separator[childValue] = input_reader.getStringRef(3).str();

            //std::string inString = input_reader.getStringRef(2).str();
            //srvInterface.log(" adding %s ", inString.c_str());
        } while (input_reader.next());
        //srvInterface.log("1");
        // exit(0);

        map<string, string> cache;
        map<string, vint> depth;
        map<string, string>::iterator p;

        for (p = label.begin(); p != label.end(); ++p) {
        	if (cache.count(p->first) == 0) {

        		string output = p->second;
        		vint current_depth = 0;
        		string current = parent[p->first];
        		while (current != "") {

#if COMPATLIB_DEBUG
        	        srvInterface.log("working on %s", current.c_str());
#endif

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

#if COMPATLIB_DEBUG
            srvInterface.log("attempting to output %s", p->first.c_str());
#endif

            switch(argTypeOID) {
            case VarcharOID: {
                VString &word = output_writer.getStringRef(0);
                word.copy(p->first);
                break;
            }
            case Int8OID:
                output_writer.setInt(0, stoll(p->first));
                break;
            default:
                break;
            }
        	output_writer.setInt(1, depth[p->first]);
			VString &word = output_writer.getStringRef(2);
			word.copy(cache[p->first]);
			output_writer.next();
        }

    }

private:
    string getArgValueAsString(PartitionReader &input_reader, size_t idx)
    {
        string value = "";
        if (!input_reader.isNull(idx)) {
            switch(argTypeOID) {
            case VarcharOID:
                value = input_reader.getStringRef(idx).str();
                break;
            case Int8OID: {
                vint intValue = input_reader.getIntRef(idx);
                value = to_string(intValue);
                break;
            }
            default:
                break;
            }
        }
        return value;
    }
};

// This factory accepts function calls with integer arguments for parent ID and child ID
class ConnectByIntFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 4 inputs (parent ID, child ID, label, separator)
	// and return a row with 3 values (id, depth, path)
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
        // Error out if we're called with anything not 4 arguments
        if (input_types.getColumnCount() != 4)
            vt_report_error(0, "Function only accepts 4 arguments, but %zu provided", input_types.getColumnCount());


        // Our output size will never be more than the input size
        output_types.addInt("identifier");
        output_types.addInt("depth");
        output_types.addVarchar(WIDTH, "path");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, ConnectBy); }

};

RegisterFactory(ConnectByIntFactory);


// This factory accepts function calls with varchar arguments for parent ID and child ID
class ConnectByVarcharFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 4 inputs (parent ID, child ID, label, separator)
	// and return a row with 3 values (id, depth, path)
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();  // parent ID
        argTypes.addVarchar();  // child ID
        argTypes.addVarchar();  // label
        argTypes.addVarchar();  // separator

        returnType.addVarchar();  // id
        returnType.addInt();      // depth
        returnType.addVarchar();  // separator
    }

    // Tell Vertica what our return string length will be, given the input string length
    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything not 4 arguments
        if (input_types.getColumnCount() != 4)
            vt_report_error(0, "Function only accepts 4 arguments, but %zu provided", input_types.getColumnCount());

        output_types.addVarchar(WIDTH, "identifier");
        output_types.addInt("depth");
        output_types.addVarchar(WIDTH, "path");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, ConnectBy); }

};

RegisterFactory(ConnectByVarcharFactory);
