/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Scalar Function: Edit distance
 *
 * Create Date: July 21, 2011
 */
#include "Vertica.h"

#include <cmath>
#include <locale> 
#include <codecvt>

using namespace Vertica;

/*
 * This is a simple function that computes the edit distance of two strings. See http://en.wikipedia.org/wiki/Levenshtein_distance
 */
class EditDistance : public ScalarFunction
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
        if (arg_reader.getNumCols() != 2)
            vt_report_error(0, "Function only accept 2 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            const std::string prePatternStr = arg_reader.getStringRef(0).str();
            const std::string preInStr = arg_reader.getStringRef(1).str();
            
            //setup converter
            using convert_type = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_type, wchar_t> converter;

            //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
            std::wstring patternStr = converter.from_bytes(prePatternStr);
            std::wstring inStr = converter.from_bytes(preInStr);

            size_t m = patternStr.size();
            size_t n = inStr.size();
            size_t d[m+1][n+1];
            for (size_t i = 0; i <= m; ++i)
                d[i][0] = i; // the distance of any first string to an empty second string
            for (size_t j = 0; j <= n; ++j)
                d[0][j] = j; // the distance of any second string to an empty first string

            for (size_t j = 1; j <= n; ++j)
                for (size_t i = 1; i <= m; ++i)
                {
                    if (patternStr[i-1] == inStr[j-1])
                        d[i][j] = d[i-1][j-1];       // no operation required
                    else
                    {
                        d[i][j] = std::min(std::min(d[i-1][j] + 1,  // a deletion
                                                    d[i][j-1] + 1),  // an insertion
                                           d[i-1][j-1] + 1); // a substitution
                    }
                }
         
            res_writer.setInt(d[m][n]);
            res_writer.next();
        } while (arg_reader.next());
    }
};

class EditDistanceFactory : public ScalarFunctionFactory
{
    // return an instance of EditDistance to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, EditDistance); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        returnType.addInt();
    }
};

RegisterFactory(EditDistanceFactory);

