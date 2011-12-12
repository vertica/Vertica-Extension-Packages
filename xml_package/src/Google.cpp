/*
 * Google.cpp
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

#include "Google.h"

const string G_AUTH_HEADER = "Auth=";
const string G_AUTH_TOKEN_PREFIX="google.auth.";

string Google::authorize(string username, string password, string service, ServerInterface &srvInterface) {

	map<string,string> variables;

	variables["Email"] = username;
	variables["Passwd"] = password;
	variables["accountType"] = "GOOGLE";
	variables["source"] = "curl-dataFeed-v2";
	variables["service"] = service;

	string vars = "";

	srvInterface.log("JJJ -----------dddd \n");

	vars = Curl::post("https://www.google.com/accounts/ClientLogin", variables, srvInterface);

	srvInterface.log("XXXXXXXX -----------dddd \n");
	srvInterface.log("XXXXXXXX ----------- %s \n", vars.c_str());

	if (!vars.empty() && vars.size() > 0) {
		string::size_type pos = vars.find(G_AUTH_HEADER);
		if ( pos != string::npos ) {
			vars = vars.substr(pos+G_AUTH_HEADER.size());
			synchronized () {
				VerticaProperties::set(G_AUTH_TOKEN_PREFIX + service, vars);
			}
		} else {
			return "Error Authentication: " + vars;
		}
	} else {
		return "Error reading client authorization.";
	}
	return vars;
}

string Google::getAuthKey(string authKey, string service) {
	if (authKey.size() > 1)
		return authKey;

	synchronized () {
		string s = VerticaProperties::get(G_AUTH_TOKEN_PREFIX + service);
		return s.size()>0?s:string("");
	}
	return NULL;
}
