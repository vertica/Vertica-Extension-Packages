/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Transform Function: Tokenize a string
 * based on a delimiter.
 *
 * Same as the StringTokenizer example we ship, but flexible delimiter and
 * returns original input as well.
 *
 * Contributed by Patrick Toole
 *
 * Create Date: Sept 13, 2011
 */
#include "Vertica.h"
#include <sstream>

using namespace Vertica;
using namespace std;


class StringTokenizerDelim : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 2)
            vt_report_error(0, "Function only accepts 2 argument, but %zu provided", input_reader.getNumCols());

        do {
            const VString &sentence = input_reader.getStringRef(0);
            const VString &delimiter = input_reader.getStringRef(1);
            const char delim = delimiter.str().c_str()[0];

            // If input string is NULL, then output is NULL as well
            if (sentence.isNull())
            {
                VString &word = output_writer.getStringRef(0);
                word.setNull();

                VString &word2 = output_writer.getStringRef(1);
                word2.setNull();

                output_writer.next();
            }
            else 
            {
                // Otherwise, let's tokenize the string and output the words
                std::string tmp = sentence.str();
                istringstream ss(tmp);
                std::string token;

                while (getline(ss, token, delim)) {
                        VString &word = output_writer.getStringRef(0);
                        word.copy(token);
                        VString &word2 = output_writer.getStringRef(1);
                        word2.copy(&sentence);
                        output_writer.next();
                }

            }
        } while (input_reader.next());
    }
};

class StringTokenizerDelimFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 2 string, and return a row with 2 strings (>string to split, >delimiter, <new word, <original word)
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();


        returnType.addVarchar();
        returnType.addVarchar();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 1 argument
        if (input_types.getColumnCount() != 2)
            vt_report_error(0, "Function only accepts 2 argument, but %zu provided", input_types.getColumnCount());

        int input_len = input_types.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "words");
        output_types.addVarchar(input_len, "input");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, StringTokenizerDelim); }

};

RegisterFactory(StringTokenizerDelimFactory);
