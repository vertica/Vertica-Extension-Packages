/*
 * Curl.h
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

#ifndef CURL_H_
#define CURL_H_


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <map>

#include <curl/curl.h>
#include "VerticaProperties.h"
#include "Vertica.h"
#include "VerticaSymbols.h"

using namespace std;

#define LOG(M) 	if (srvI) srvI->log(M); else printf(M)

const string CURL_INITED = "curl.initialized";

class Curl {

private:
	static string merge_data();
	static void initialize();
	static string fetch(string url, map<string,string> &variables, map<string, string> &headers, int isPost);

public:
//	static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
	static string post(string url, map<string,string> &variables, Vertica::ServerInterface &srvInterface);
	static string post(string url, map<string,string> &variables);
	static string get(string url, map<string, string> &headers, Vertica::ServerInterface &srvInterface);
	static string get(string url, map<string, string> &headers);
	static string get(string url, Vertica::ServerInterface &srvInterface);
	static string get(string url);
};

#endif /* CURL_H_ */
