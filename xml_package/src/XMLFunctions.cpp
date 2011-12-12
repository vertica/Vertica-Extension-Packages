/*
 * XMLFunctions.cpp
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

#include "XMLFunctions.h"


string XMLFunctions::xslt(string &data, string &style, ServerInterface &srvInterface) {
    xmlDocPtr doc; /* the resulting document tree */
    xmlDocPtr styeSheet; /* the resulting document tree */

    xsltStylesheetPtr ssPtr;
	const char *params[1];
	int nbparams = 0;
	params[nbparams] = NULL;

    doc = xmlReadMemory(data.c_str(), data.size(), "noname.xml", NULL, 0);
    // xmlNode *root_element = NULL;
    // root_element = xmlDocGetRootElement(doc);
    // print_element_names(root_element);

    styeSheet = xmlReadMemory(style.c_str(), style.length(), "noname.xml", NULL, 0);

    ssPtr = xsltParseStylesheetDoc (styeSheet);
    xmlDocPtr result = xsltApplyStylesheet(ssPtr, doc, params);

    xmlChar * resultString;
    int stringLen;

    xsltSaveResultToString(&resultString, &stringLen, result, ssPtr);
    return string((char*) resultString);
}

ResponseInfo XMLFunctions::xpath(string &data, string &path, ServerInterface &srvInterface) {
    xmlDocPtr doc; /* the resulting document tree */
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;


    doc = xmlReadMemory(data.c_str(), data.size(), "noname.xml", NULL, 0);
    // xmlNode *root_element = NULL;
    // root_element = xmlDocGetRootElement(doc);
    // print_element_names(root_element);

    xpathCtx = xmlXPathNewContext(doc);
    xpathObj = xmlXPathEvalExpression((unsigned char *) path.c_str(), xpathCtx);

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;

    ResponseInfo responses;
    if (size>0) {
		int next=0;
		for(int i = 0; i< size; i ++) {
			if(xmlNodeGetContent(nodes->nodeTab[i])) {
				responses[next] = string((char*) xmlNodeGetContent(nodes->nodeTab[i]));
				next++;
			}
		}
    }
    return responses;
}


class XSLTApply : public ScalarFunction
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
            if (arg_reader.getStringRef(0).isNull()) {
                res_writer.getStringRef().setNull();
            }
            else {

            	string data = arg_reader.getStringRef(0).str();
            	string style = arg_reader.getStringRef(1).str();
            	res_writer.getStringRef().copy(
            			XMLFunctions::xslt(data, style, srvInterface)
            	);
            }

            res_writer.next();
        } while (arg_reader.next());
    }
};


class XPathFirst : public ScalarFunction
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
            if (arg_reader.getStringRef(0).isNull()) {
                res_writer.getStringRef().setNull();
            }
            else {

            	string data = arg_reader.getStringRef(0).str();
            	string path = arg_reader.getStringRef(1).str();
            	ResponseInfo response = XMLFunctions::xpath(data, path, srvInterface);
            	if ((int) response.size() < arg_reader.getIntRef(2)) {
            		res_writer.getStringRef().setNull();
            	} else if ((int) response.size() > 0) {
					res_writer.getStringRef().copy( response[arg_reader.getIntRef(2)-1]);
            	} else {
            		res_writer.getStringRef().setNull();
            	}
            }

            res_writer.next();
        } while (arg_reader.next());
    }
};


class XPathFind : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        do {
            if (input_reader.getStringRef(0).isNull()) {
            	output_writer.getStringRef(0).setNull();
            }
            else {
            	string data = input_reader.getStringRef(0).str();
            	string path = input_reader.getStringRef(1).str();

            	ResponseInfo response = XMLFunctions::xpath(data, path, srvInterface);
            	if (response.size() > 0) {
            		for (int i=0;i<(int) response.size();i++) {
                		output_writer.getStringRef(0).copy(response[i]);
                        output_writer.next();
            		}
            	} else {
            		output_writer.getStringRef(0).setNull();
                    output_writer.next();
            	}
            }

        } while (input_reader.next());
    }

};


class XSLTApplySplit : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        do {
            if (input_reader.getStringRef(0).isNull()) {
            	output_writer.getStringRef(0).setNull();
            }
            else {
            	string data = input_reader.getStringRef(0).str();
            	string style = input_reader.getStringRef(1).str();

    			string response = XMLFunctions::xslt(data, style, srvInterface);
    			SplitMap map = VerticaProperties::split(response,input_reader.getStringRef(2).str(), input_reader.getStringRef(3).str());

    			for (int i=0; i<(int) map.size(); i++) {
    				for (int j=0;j<min(5,(int)map[i].size());j++) {
						output_writer.getStringRef(j).copy(map[i][j]);
    				}
    				output_writer.next();
    			}
            }
        } while (input_reader.next());
    }

};


class XSLTApplyFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, XSLTApply); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)

    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        returnType.addVarchar();
    }


    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        returnType.addVarchar(64000);
    }
};


class XPathNumFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, XPathFirst); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)

    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        argTypes.addInt();
        returnType.addVarchar();
    }


    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        returnType.addVarchar(64000);
    }
};



class XPathFindFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
    	argTypes.addVarchar();
    	argTypes.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        output_types.addVarchar(4000, "xpath_result");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, XPathFind); }

};


class XSLTApplySplitFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
    	argTypes.addVarchar();
    	argTypes.addVarchar();
    	argTypes.addVarchar();
    	argTypes.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        output_types.addVarchar(4000, "xstl_split_1");
        output_types.addVarchar(4000, "xstl_split_2");
        output_types.addVarchar(4000, "xstl_split_3");
        output_types.addVarchar(4000, "xstl_split_4");
        output_types.addVarchar(4000, "xstl_split_5");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, XSLTApplySplit); }

};


RegisterFactory(XPathNumFactory);
RegisterFactory(XPathFindFactory);
RegisterFactory(XSLTApplyFactory);
RegisterFactory(XSLTApplySplitFactory);
