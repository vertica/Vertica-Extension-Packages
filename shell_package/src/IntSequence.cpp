/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: User Defined Transform Function that generates sequences of integers
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"

using namespace Vertica;

/*
 * generates sequences of numbers (start,end)
 */
class IntSequence : public TransformFunction
{
public:

    /*
     * For each partition, outputs all integers between and including first and second values
     */
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        // Basic error checking
        if (input_reader.getNumCols() != 2)
            vt_report_error(0, "Function only accept 2 arguments, but %zu provided", 
                            input_reader.getNumCols());

        // While we have inputs to process
        do {
            vint first = input_reader.getIntRef(0);
            vint second = input_reader.getIntRef(1);

            vint inc = (first < second) ? 1 : -1;

            while (first != second)
            {
                // Copy string into results
                output_writer.setInt(0, first);
                output_writer.next();
                first += inc;
            }
            output_writer.setInt(0, first);
            output_writer.next();
        } while (input_reader.next());
    }
};

class IntSequenceFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 2 strings
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addInt();
        argTypes.addInt();

        returnType.addInt();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 1 argument
        if (input_types.getColumnCount() != 2)
            vt_report_error(0, "Function only accepts 2 arguments, but %zu provided", input_types.getColumnCount());

        output_types.addArg(input_types.getColumnType(0), "val");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, IntSequence); }

};

RegisterFactory(IntSequenceFactory);

