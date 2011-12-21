/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: User Defined Scalar Function that returns hostname of machine on which it is invoked
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"
#include <unistd.h>

using namespace Vertica;

#define HOST_NAME_MAX 100

/*
 * This is a simple function that removes all spaces from the input string
 */
class Hostname : public ScalarFunction
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
        if (arg_reader.getNumCols() != 0)
            vt_report_error(0, "Function only accept 0 argument, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        char hostname[HOST_NAME_MAX];
        gethostname(hostname,HOST_NAME_MAX);
        hostname[HOST_NAME_MAX-1] = '\0'; // ensure null terminated
        do {
            // Copy string into results
            res_writer.getStringRef().copy(hostname);
            res_writer.next();
        } while (arg_reader.next());
    }
};

class HostnameFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, Hostname); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
         returnType.addVarchar(HOST_NAME_MAX);
    }
};

RegisterFactory(HostnameFactory);

