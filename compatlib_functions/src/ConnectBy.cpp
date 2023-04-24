#include "Vertica.h"
#include <string>
#include <map>

using namespace Vertica;
using namespace std;


const int WIDTH = 65000;

class ConnectBy : public TransformFunction
{
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
    	if (input_reader.getNumCols() != 4)
            vt_report_error(0, "Function only accepts 4 argument, but %zu provided", input_reader.getNumCols());

        switch(argTypeOID) {
        case VarcharOID:
            internalProcessPartition<string>(srvInterface, input_reader, output_writer);
            break;
        case Int8OID:
            internalProcessPartition<vint>(srvInterface, input_reader, output_writer);
            break;
        default:
            break;
        }
    }

private:

    BaseDataOID argTypeOID;  // data type OID of parent ID and child ID

    bool isEndValue(vint value)
    {
        return value == vint_null;
    }

    bool isEndValue(string value)
    {
        return value == "";
    }

    void getValueFromReader(vint &output, PartitionReader &input_reader, size_t idx)
    {
        output = input_reader.getIntRef(idx);
    }

    void getValueFromReader(string &output, PartitionReader &input_reader, size_t idx)
    {
        output = input_reader.getStringRef(idx).str();
    }

    void setValueToWriter(vint value, PartitionWriter &output_writer, size_t idx)
    {
        output_writer.setInt(idx, value);
    }

    void setValueToWriter(string value, PartitionWriter &output_writer, size_t idx)
    {
        VString &word = output_writer.getStringRef(idx);
        word.copy(value);
    }

#if COMPATLIB_DEBUG
    void log(ServerInterface &srvInterface, const char *format, string value)
    {
        srvInterface.log(format, value.c_str());
    }

    void log(ServerInterface &srvInterface, const char *format, vint value)
    {
        log(srvInterface, format, to_string(value));
    }
#endif

    template <typename T>
    void internalProcessPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
    	map<T, T> parent;
    	map<T, string> label;
    	map<T, string> separator;

        do {
            T childValue;
            // If child ID is null, skip it
            if (input_reader.isNull(1)) {
                continue;
            } else {
                getValueFromReader(childValue, input_reader, 1);
                getValueFromReader(parent[childValue], input_reader, 0);
            }

            getValueFromReader(label[childValue], input_reader, 2);
            getValueFromReader(separator[childValue], input_reader, 3);
        } while (input_reader.next());

        map<T, string> cache;
        map<T, vint> depth;
        typename map<T, string>::iterator p;

        for (p = label.begin(); p != label.end(); ++p) {

#if COMPATLIB_DEBUG
            log(srvInterface, "working on [%s]", p->first);
#endif

        	if (cache.count(p->first) == 0) {
        		string output = p->second;
        		vint current_depth = 0ull;
        		T current = parent[p->first];


#if COMPATLIB_DEBUG
                log(srvInterface, "  parent: [%s]", current);
#endif

        		while (!isEndValue(current)) {
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

#if COMPATLIB_DEBUG
                log(srvInterface, "    output: [%s]", output);
                log(srvInterface, "    depth : [%s]", current_depth);
#endif

        	}

            setValueToWriter(p->first, output_writer, 0);
            setValueToWriter(depth[p->first], output_writer, 1);
            setValueToWriter(cache[p->first], output_writer, 2);
			output_writer.next();
        }

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
