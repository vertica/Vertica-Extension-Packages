/*
 * Curl.cpp
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

#include "Curl.h"
#include "VerticaSymbols.h"

struct MemoryStruct {
	char *memory;
	size_t size;
};



using namespace std;
using namespace Vertica;

//string url, string[] variableNames, string[] variableValues

void **pointers = new void *[10000];
int* sizes = new int[10000];
int nextPtr = 0;
char *data;
int totalSize = 0;
Vertica::ServerInterface* srvI = NULL;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	pointers[nextPtr] = new char[nmemb];
	memcpy(pointers[nextPtr],ptr,nmemb);
	sizes[nextPtr] = nmemb;
	nextPtr++;

	return sizes[nextPtr-1];
}

static void flush_data() {
	for (int i=0;i<nextPtr;i++) {
		free(pointers[i]);
	}
	nextPtr = 0;
	free(data);
	totalSize = 0;
}

string Curl::merge_data() {

	totalSize = 0;
	for (int i=0;i<nextPtr;i++) {
		totalSize += sizes[i];
	}


	data = new char[totalSize+1];
	data[totalSize]=0;
	int start = 0;

	for (int i=0;i<nextPtr;i++) {
		memcpy(&data[start], pointers[i], sizes[i]);
		start += sizes[i];
	}

	return string(data)+"\0";
}

void Curl::initialize() {
	synchronized() {
		if (VerticaProperties::get(CURL_INITED).empty() ||
				VerticaProperties::get(CURL_INITED) != "T") {
			curl_global_init(CURL_GLOBAL_ALL);
			VerticaProperties::set(CURL_INITED, "T");
		}
	}
}

string Curl::post(string url, map<string,string> &variables, Vertica::ServerInterface &srvInterface) {
	srvI = &srvInterface;
	return post(url, variables);
}


string Curl::get(string url, Vertica::ServerInterface &srvInterface) {
	srvI = &srvInterface;
	return get(url);
}

string Curl::get(string url) {
	map<string,string> headers;
	map<string,string> noVars;
	return fetch(url,noVars, headers, 0);
}

string Curl::get(string url, map<string, string> &headers, Vertica::ServerInterface &srvInterface) {
	srvI = &srvInterface;

	return get(url, headers);
}

string Curl::get(string url, map<string, string> &headers) {
	map<string,string> noVars;
	return fetch(url,noVars, headers, 0);
}

string Curl::post(string url, map<string,string> &variables) {
	map<string,string> headers;
	return fetch(url,variables, headers, 1);
}

string Curl::fetch(string url, map<string,string> &variables, map<string, string> &headers, int isPost) {

	string s = "";

	CURL *curl;
	CURLcode res;

	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	struct curl_slist *chunk = NULL;

	if (headers.size() > 0) {
		for( map<string,string>::iterator iter = headers.begin(); iter != headers.end(); iter++ ) {
			string nextHeader = string(iter->first+string(": ")+iter->second);
			chunk = curl_slist_append(chunk, nextHeader.c_str());
		}
	}

	// curl_global_init(CURL_GLOBAL_ALL);
	initialize();

	for( map<string,string>::iterator iter = variables.begin(); iter != variables.end(); iter++ ) {
		/* Fill in the file upload field */
		curl_formadd(&formpost,
				&lastptr,
				CURLFORM_COPYNAME, iter->first.c_str(), //the name of the data to send
				CURLFORM_COPYCONTENTS, iter->second.c_str(), //the users name
				CURLFORM_END);
	}

	curl = curl_easy_init();

	if(curl) {
		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		if (headers.size() > 0) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		}

		/* no progress meter please */
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

		if (isPost) {
			/* initialize custom header list (stating that Expect: 100-continue is not wanted */
			headerlist = curl_slist_append(headerlist, buf);

			curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		}

		synchronized() {
			/* The reason we have this section is that the writeback function does not appear to have a way
			 * to operate in a non-static method; this entire function probably should not be an instance
			 * method because of that.
			 */

			flush_data();

			res = curl_easy_perform(curl);

			if (res > 0) {

				char buf[2000] =  {'\0'};
				sprintf(buf, "ERROR executing CURL: %d\n", res);
				s = string(buf);
			} else {
				s = merge_data();
			}

		}

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the formpost chain */
		curl_formfree(formpost);

		/* free slist */
		curl_slist_free_all (headerlist);
		/* free slist */
		curl_slist_free_all (chunk);

	}


	return s;
}


class HttpGet : public ScalarFunction
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

            	res_writer.getStringRef().copy(Curl::get(arg_reader.getStringRef(0).str(), srvInterface));
            }

            res_writer.next();
        } while (arg_reader.next());
    }
};


class HttpGetFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, HttpGet); }

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
        returnType.addVarchar(64000);
    }
};

RegisterFactory(HttpGetFactory);

