/*
 * Google.h
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

#ifndef GOOGLE_H_
#define GOOGLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <map>

#include <curl/curl.h>
#include "Vertica.h"
#include "VerticaProperties.h"
#include "Curl.h"

using namespace std;
using namespace Vertica;

const string GOOGLE_SERVICE_ANALYTICS = "analytics";

class Google {
public:
	static string authorize(string username, string password, string service, ServerInterface &srvInterface);
	static string getAuthKey(string authKey, string service);
};

#endif /* GOOGLE_H_ */
