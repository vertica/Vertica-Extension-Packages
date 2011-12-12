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

#ifndef VERTICAPROPERTIES_H_
#define VERTICAPROPERTIES_H_

#include <pthread.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <pthread.h>

#include "Vertica.h"

// #define synchronized()  for(int ixi=!pthread_mutex_lock(VerticaProperties::mutex());;ixi=pthread_mutex_unlock(VerticaProperties::mutex()))
#define synchronized()  if(1)

using namespace std;



typedef map<int, map<int, string> > SplitMap;


class VerticaStatics {
public:
	pthread_mutex_t verticaMutex;

	VerticaStatics() {
		pthread_mutexattr_t    attr;
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&verticaMutex, &attr);
	}
};

class VerticaProperties {


protected:
	static VerticaStatics staticVariables;
	static map<string,string> verticaProperties;


public:
	static map<string,string>  getMap();

	static void set(string key, string value);
	static string get(string key);
	static pthread_mutex_t* mutex();

	static SplitMap split(string data,string field_delimiter, string record_delimiter);

};


#endif /* VERTICAPROPERTIES_H_ */
