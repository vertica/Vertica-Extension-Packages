/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: Functions that help with full-text search over documents
 *
 * Create Date: Nov 25, 2011
 */
#include <sstream>
#include "Vertica.h"
#include "libstemmer.h"

using namespace Vertica;
using namespace std;

/*
 * This is a simple function that stems an input word
 */
class Stem : public ScalarFunction
{
    sb_stemmer *stemmer;

public:
    // Set up the stemmer
    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        stemmer = sb_stemmer_new("english", NULL);
    }

    // Destroy the stemmer
    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        sb_stemmer_delete(stemmer);
    }

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

            const unsigned char* stemword = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(inStr.c_str()), inStr.size());

            // Copy string into results
            res_writer.getStringRef().copy(reinterpret_cast<const char*>(stemword));
            res_writer.next();
        } while (arg_reader.next());
    }
};

class StemFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, Stem); }

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

RegisterFactory(StemFactory);

// Fetches a document using CURL and tokenizes it
class DocTokenizer : public TransformFunction
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

class DocTokenFactory : public TransformFunctionFactory
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
    { return vt_createFuncObj(srvInterface.allocator, DocTokenizer); }

};

RegisterFactory(DocTokenFactory);
