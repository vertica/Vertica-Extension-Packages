/*
Portions of this software Copyright (c) 2011 by Vertica, an HP
Company.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Vertica.h"
#include <stdio.h>
#include "my_aes.h"


using namespace Vertica;

/*
 * This is a simple function that removes all spaces from the input string
 */
class AESDecrypt: public ScalarFunction
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
        if (arg_reader.getNumCols() != 2) //fixme: accept key as a parm
            vt_report_error(0, "Function accepts exactly 2 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
	    char *t;
	    unsigned int l;
	    int r=0;
            std::string inStr = arg_reader.getStringRef(0).str();
	    std::string kStr = arg_reader.getStringRef(1).str();

	    l=inStr.capacity(); // warning, this might be a hack
	    l=inStr.length(); // warning, this might be a hack

	    t=(char *)malloc(l+1); // add one for the null.
	    memset(t,0,l+1);
	    
	    r=my_aes_decrypt(inStr.c_str(),l,t,kStr.c_str(),kStr.length());

	    // Few debug loggers
            //srvInterface.log("AESDecrypt: InLen: ->%d<-",l);
            //srvInterface.log("AESDecrypt: Input: ->%s<-",inStr.c_str());
            //srvInterface.log("AESDecrypt: Outpt: ->%s<-",t);
            //srvInterface.log("AESDecrypt: OutLn: ->%d<-",r);

	    if (r < 0) {
		srvInterface.log("AESDecrypt: Error decrypting: %d",r);
		return;
	    }
	    // We try to preserve the input string in it's entirety before encrypting it, this includes
	    // nulls. So the decrypted string should already be null terminated. But you never know. So we tack 
	    // one on the end anyway.
	    t[r+1]=0;
	    res_writer.getStringRef().copy(t,r);
	    free(t);
	    res_writer.next();
	    
        } while (arg_reader.next());
    }
};

class AESDecryptFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, AESDecrypt); }

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
        const VerticaType &t = argTypes.getColumnType(0);
        returnType.addVarchar(t.getStringLength());
    }
};

RegisterFactory(AESDecryptFactory);

