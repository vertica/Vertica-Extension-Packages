/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: User Defined Transform Function: for each partition, output a
 * list as a string separated by a custom delimiter
 *
 * Create Date: Dec 15, 2011
 */
#include "Vertica.h"
#include <sstream>
#include <string>

using namespace Vertica;
using namespace std;

#define LINE_MAX 64000

/*
 * Same as the group_concat in the same library, but with a flexible delimiter.
 * Takes in a sequence of string values and a delimiter character and produces a single output tuple with
 * a list of values separated by the delimiter.  If the output string would overflow the
 * maximum line length, stop appending values and include a "..."
 */

class GroupConcatDelim : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 2)
            vt_report_error(0, "Function only accepts 2 argument, but %zu provided", input_reader.getNumCols());

        ostringstream oss;
        bool first = true;
        bool exceeded = false;
        do {
            const VString &elem = input_reader.getStringRef(0);
            const VString &delimiter = input_reader.getStringRef(1);
            const char delim = delimiter.str().c_str()[0];

            // If input string is NULL, then ignore it
            if (elem.isNull())
            {
                continue;
            }
            else if (!exceeded)
            {
                std::string s = elem.str();
                size_t curpos = oss.tellp();
                curpos += s.length() + 2;
                if (curpos > LINE_MAX)
                {
                    exceeded = true;
                    if (first) oss << "...";
                    else oss << delim << "...";
                }
                else
                {
                    if (!first) oss << delim;
                    first = false;
                    oss << s;
                }
            }
        } while (input_reader.next());

        VString &summary = output_writer.getStringRef(0);
        summary.copy(oss.str().c_str());
        output_writer.next();
    }
};

class GroupConcatDelimFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 1 string
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();

        returnType.addVarchar();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 2 argument
        if (input_types.getColumnCount() != 2)
            vt_report_error(0, "Function only accepts 2 argument, but %zu provided", input_types.getColumnCount());

        // output can be wide.  Include extra space for a last "..."
        output_types.addVarchar(LINE_MAX+5, "list");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, GroupConcatDelim); }

};

RegisterFactory(GroupConcatDelimFactory);