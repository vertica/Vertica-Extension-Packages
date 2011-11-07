/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: Example User Defined Transform Function: Tokenize a string
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"
#include <sstream>

using namespace Vertica;
using namespace std;


/*
 * String tokenizer - This transform function takes in a string argument and
 * tokenizes it. For example, it will turn the following input:
 *
 *       url           description
 * ----------------+------------------------------
 * www.amazon.com  | Online retailer
 * www.hp.com      | Major hardware vendor
 *
 * into the following output:
 *
 *       url           words
 * ----------------+------------------
 * www.amazon.com  | Online
 * www.amazon.com  | retailer
 * www.hp.com      | Major
 * www.hp.com      | hardware
 * www.hp.com      | vendor
 *
 */

class StringTokenizer : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

        do {
            const VString &sentence = input_reader.getStringRef(0);

            // If input string is NULL, then output is NULL as well
            if (sentence.isNull())
            {
                VString &word = output_writer.getStringRef(0);
                word.setNull();
                output_writer.next();
            }
            else 
            {
                // Otherwise, let's tokenize the string and output the words
                std::string tmp = sentence.str();
                istringstream ss(tmp);

                do
                {
                    std::string buffer;
                    ss >> buffer;
                    
                    // Copy to output
                    if (!buffer.empty()) {
                        VString &word = output_writer.getStringRef(0);
                        word.copy(buffer);
                        output_writer.next();
                    }
                } while (ss);
            }
        } while (input_reader.next());
    }
};

class StringTokenizerFactory : public TransformFunctionFactory
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

        int input_len = input_types.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "words");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, StringTokenizer); }

};

RegisterFactory(StringTokenizerFactory);
