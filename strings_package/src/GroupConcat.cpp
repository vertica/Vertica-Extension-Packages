/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: User Defined Transform Function: for each partition, output a comma-separated list in a string
 *
 * Create Date: Dec 15, 2011
 */
#include "Vertica.h"
#include <sstream>

using namespace Vertica;
using namespace std;


/*
 * Takes in a sequence of string values and produces a single output tuple with
 * a comma separated list of values.
 */

class GroupConcat : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

        ostringstream oss;
        bool first = true;
        do {
            const VString &elem = input_reader.getStringRef(0);

            // If input string is NULL, then ignore it
            if (elem.isNull())
            {
                continue;
            }
            else 
            {
                if (!first) oss << ", ";
                first = false;
                oss << elem.str();
            }
        } while (input_reader.next());

        VString &summary = output_writer.getStringRef(0);
        summary.copy(oss.str().c_str());
        output_writer.next();
    }
};

class GroupConcatFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 1 string
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();

        returnType.addVarchar();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 1 argument
        if (input_types.getColumnCount() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_types.getColumnCount());

        // Our output size will never be more than the input size
        output_types.addVarchar(65000, "summary");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, GroupConcat); }

};

RegisterFactory(GroupConcatFactory);
