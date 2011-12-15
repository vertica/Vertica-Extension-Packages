/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: Example User Defined Scalar Function: Remove spaces
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"

using namespace Vertica;

/*
 * This is a simple function that removes all spaces from the input string
 */
class RemoveSpace : public ScalarFunction
{
public:

    /*
     * This method processes a block of rows in a single invocation.
     *
     * The inputs are retrieved via arg_reader
     * The outputs are returned via arg_writer
     */
    virtual void processBlock(ServerInterface &srvInterface,
                              BlockReader &arg_reader,
                              BlockWriter &res_writer)
    {
        // Basic error checking
        if (arg_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            std::string  inStr = arg_reader.getStringRef(0).str();
            char buf[inStr.size() + 1];


            // copy data from inStr to buf one character at a time
            const char *src = inStr.c_str();
            int len = 0;
            while (*src) {
                if (*src != ' ') buf[len++] = *src;
                src++;
            }
            buf[len] = '\0'; // null termiante

            // Copy string into results
            res_writer.getStringRef().copy(buf);
            res_writer.next();
        } while (arg_reader.next());
    }
};

class RemoveSpaceFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, RemoveSpace); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        const VerticaType &t = argTypes.getColumnType(0);
        returnType.addVarchar(t.getStringLength());
    }
};

RegisterFactory(RemoveSpaceFactory);

