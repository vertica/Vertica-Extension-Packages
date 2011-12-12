/*
 * VerticaProperties.h
 *
 *  Created on: Dec 5, 2011
 *      Author: ptoole
 *
 * Vertica Analytic Database
 *
 * Makefile to build package directory
 *
 * Copyright 2011 Vertica Systems, an HP Company
 *
 *
 * Portions of this software Copyright (c) 2011 by Vertica, an HP
 * Company.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef VERTICAPROPERTIES_C_
#define VERTICAPROPERTIES_C_

#include "VerticaProperties.h"

using namespace std;
using namespace Vertica;

map<string,string> VerticaProperties::verticaProperties;
VerticaStatics VerticaProperties::staticVariables;



int WIDTH = 2000;

void VerticaProperties::set(string key, string value) {
	VerticaProperties::verticaProperties[key] = value;
}


string VerticaProperties::get(string key) {
	return VerticaProperties::verticaProperties[key];
}


/*
 * Find is lacking in many ways. This is a workaround.
 */
int ireplace (int val, int test, int sub) {
	if (val == test)
		return sub;
	if (sub < val)
		return sub;

	return val;
}

SplitMap VerticaProperties::split(string data,string field_delimiter, string record_delimiter) {

	SplitMap map;

	int row = 0;
	int currentRowStart = 0;
	int currentRowEnd = ireplace((int) data.find(record_delimiter), (int) string::npos, (int) data.size());
	do {
		int column = 0;
		int currentFieldStart = currentRowStart;
		int currentFieldEnd = ireplace( (int) data.find(field_delimiter.c_str(), currentFieldStart),
				(int) string::npos, currentRowEnd);
		do {
			map[row][column] = data.substr(currentFieldStart, currentFieldEnd-currentFieldStart);
			column++;
			currentFieldStart = currentFieldEnd + field_delimiter.size();
			currentFieldEnd = ireplace((int) data.find(field_delimiter.c_str(), currentFieldStart),
					(int) string::npos, currentRowEnd);

		} while (currentFieldStart < min((int) data.size(), currentRowEnd));

		currentRowStart = currentRowEnd + record_delimiter.size();
		currentRowEnd = ireplace((int) data.find(record_delimiter, currentRowStart),
				(int) string::npos, (int) data.size());

		row++;
	} while (currentRowStart < (int) data.size());
	return map;
}

map<string,string> VerticaProperties::getMap() {
	return verticaProperties;
}

pthread_mutex_t* VerticaProperties::mutex() {
	return &(staticVariables.verticaMutex);
}


class VerticaPropSet : public ScalarFunction
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

        	VerticaProperties::set(arg_reader.getStringRef(0).str(), arg_reader.getStringRef(1).str());
        	res_writer.setInt(1);

            res_writer.next();
        } while (arg_reader.next());
    }
};


class VerticaPropGet : public ScalarFunction
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

        	res_writer.getStringRef().copy(
//        		VerticaProperties::verticaProperties[arg_reader.getStringRef(0).str()]
				VerticaProperties::get(arg_reader.getStringRef(0).str())
        	);

            res_writer.next();
        } while (arg_reader.next());
    }
};


class VerticaListProp : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
    	for( map<string,string>::iterator iter = VerticaProperties::getMap().begin(); iter != VerticaProperties::getMap().end(); iter++ ) {
    		/* Fill in the file upload field */
    		output_writer.getStringRef(0).copy(iter->first);
    		output_writer.getStringRef(1).copy(iter->second);
    		output_writer.next();
    	}
    }

};


class VerticaPropSetFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, VerticaPropSet); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)

    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        returnType.addInt();
    }

};


class VerticaPropGetFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, VerticaPropGet); }

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
        returnType.addVarchar(2000);
    }
};

class VerticaListPropFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        returnType.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        output_types.addVarchar(WIDTH, "name");
        output_types.addVarchar(WIDTH, "value");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, VerticaListProp); }

};


RegisterFactory(VerticaPropSetFactory);
RegisterFactory(VerticaPropGetFactory);
RegisterFactory(VerticaListPropFactory);


#endif /* VERTICAPROPERTIES_C_ */
