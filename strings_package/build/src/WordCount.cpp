/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Scalar Function: Edit distance
 *
 * Create Date: July 21, 2011
 */
#include "Vertica.h"

using namespace Vertica;

/*
 * This is a simple function that computes the number of words in a string
 */
class WordCount : public ScalarFunction
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
        // While we have inputs to process
        do {
            if (arg_reader.getStringRef(0).isNull()) 
            { 
                res_writer.getStringRef().setNull(); 
            }
            else 
            {
                const char *DELIMS = " \t\n"; // break on \t and \n
                const VString &s = arg_reader.getStringRef(0);
                size_t sz = s.length();

                // Get a copy of the input string
                char *inStr = (char*)malloc(sz + 1);
                memcpy(inStr, s.data(), sz);
                inStr[sz] = '\0'; 

                char *saveptr;
                vint cnt = 0;

                // use strtok to find the tokens
                for (const char *tok = strtok_r(inStr, DELIMS, &saveptr); 
                     tok; tok = strtok_r(NULL, DELIMS, &saveptr)) 
                {
                    cnt++;
                }
                free(inStr);
                
                res_writer.setInt(cnt);
            }

            res_writer.next();
        } while (arg_reader.next());
    }
};

class WordCountFactory : public ScalarFunctionFactory
{
    // return an instance of WordCount to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, WordCount); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addInt();
    }
};

RegisterFactory(WordCountFactory);

