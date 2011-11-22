/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Transform Function: Extract pairs of name/value from a http uri string
 *
 * Create Date: June 06, 2011
 */

//third party lib: uriparse lib
#include <uriparser/Uri.h>

#include "Vertica.h"
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

using namespace Vertica;
using namespace std;

/*
 * HTTP URI extractor - This transform function takes in a string argument and
 * extractes the name/value pairs. For example, it will turn the following input:
 *
 *                  url
 * ------------------------------------------------------------------------------------------------
 * http://www.google.com/search?q=lm311+video+sync+schematic&hl=id&lr=&ie=UTF-8&start=10&sa=N 
 * http://www.yahoo.com/search?yq=sync+schematic%3F&ie=UTF-8 
 *
 * into the following output:
 *
 *       name           value
 * ----------------+------------------
 * q		   | lm311 video sync schematic
 * hl              | id
 * lr              | 
 * ie              | UTF-8
 * start           | 10
 * sa              | N
 * yq              | sync schematic?
 * ie              | UTF-8 
 *
 */

class UriExtractor : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 1) 
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

        do {

        	const VString &inputVString = input_reader.getStringRef(0);

        	// If input string is NULL, then output is NULL as well
        	if (inputVString.isNull()) {

                VString & name = output_writer.getStringRef(0);
                VString & value = output_writer.getStringRef(1);
                name.setNull();
                value.setNull();
                output_writer.next();

        	} else {

                std::string inputString = inputVString.str();

                //extract pairs of (name, value)
                std::vector<PairString> pairs;
                getPairs(inputString, pairs);

                for (unsigned int i = 0; i < pairs.size(); i++) {
                    VString & name = output_writer.getStringRef(0);
                    VString & value = output_writer.getStringRef(1);
                    PairString & pair = pairs[i];
                    if (!pair._name.empty()) {
                      	name.copy(pair._name);
                    } else {
                      	name.setNull();
                    }
                    if (!pair._value.empty()) {
                      	value.copy(pair._value);
                    } else {
                       	value.setNull();
                    }
                    output_writer.next();
               }
        	}
        } while (input_reader.next());
    }

    struct PairString {
    	std::string _name;
    	std::string _value;
    	PairString() : _name(), _value() {}
    	PairString(const char * name, const char * value) : _name(name), _value(value) {}
    	PairString(const std::string & name, const std::string & value) : _name(name), _value(value) {}
    };

    bool validURI(UriUriA & uri) {
    	if (uri.scheme.first == NULL || uri.scheme.first == uri.scheme.afterLast) {
    		return false;
    	}
    	if (uri.hostText.first == NULL || uri.hostText.first == uri.hostText.afterLast) {
    		return false;
    	}
    	return true;
    }

    //extract pairs of (name, value)
    void getPairs(const std::string & queryURL, std::vector<PairString> & pairs) {

    	pairs.clear();

    	if (queryURL.empty()) {
    		pairs.push_back(PairString(std::string(), std::string()));
    		return;
    	}

    	const char * bptr = queryURL.c_str();
    	const char * eptr = bptr + queryURL.length();

    	UriParserStateA uriParserState;
    	UriUriA uri;
    	uriParserState.uri = &uri;

    	//parse the uri by calling the function in the uriparser lib
    	if (uriParseUriExA(&uriParserState, bptr, eptr) != URI_SUCCESS || !validURI(uri)) {
    		uriFreeUriMembersA(&uri);
    		pairs.push_back(PairString(std::string(), std::string()));
    		return;
    	}

        bptr = uri.scheme.first;
        eptr = uri.scheme.afterLast;
    	//check if the uri is http uri
       	if (bptr == NULL || bptr == eptr ||
       		!(std::string(bptr, eptr - bptr) == "http")) {
     		uriFreeUriMembersA(&uri);
     		pairs.push_back(PairString(std::string(), std::string()));
    		return;
    	}

       	//parse the query uri
    	UriQueryListA * queryList = NULL;
        int itemCount;

        bptr = uri.query.first;
        eptr = uri.query.afterLast;
        //calling the function in the uriparser lib
        if (bptr != NULL && bptr != eptr &&
        	uriDissectQueryMallocA(&queryList, &itemCount, bptr, eptr) == URI_SUCCESS) {
        	pairs.reserve(itemCount);
          	UriQueryListA * itemPtr = queryList;
        	for (int i = 0; i < itemCount && itemPtr != NULL; i++, itemPtr = itemPtr->next) {
        		pairs.push_back(PairString(itemPtr->key ? itemPtr->key : "", itemPtr->value ? itemPtr->value : ""));
        	}
        }
        if (queryList) {
        	uriFreeQueryListA(queryList);
        }
        uriFreeUriMembersA(&uri);

        if (pairs.empty()) {
        	pairs.push_back(PairString(std::string(), std::string()));
        }
    	return;
    }
				
};

class UriExtractorFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 2 strings
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
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
        if (input_types.getColumnCount() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_types.getColumnCount());

        int input_len = input_types.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "name");
        output_types.addVarchar(input_len, "value");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { 
    	return vt_createFuncObj(srvInterface.allocator, UriExtractor);
    }

};

RegisterFactory(UriExtractorFactory);
