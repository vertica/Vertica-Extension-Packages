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
class AESEncrypt: public ScalarFunction
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
            vt_report_error(0, "Function accepts exactly 2 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
	    char *t;
	    int r=0;
	    unsigned int l=0;

            std::string inStr = arg_reader.getStringRef(0).str();
            std::string kStr = arg_reader.getStringRef(1).str();

	    l=inStr.size();
	   
	    //l=strlen(inStr.c_str())+1; // we want to include the null
	    //s=(char *)alloc(l+1); // malloc enough for the null terminator

	    t=(char *)malloc(l+AES_BLOCK_SIZE); // aes encrypts in 16 byte blocks. alloc enough for padding

	    memset(t,0,l+16); // clear with nulls
	   
	    r=my_aes_encrypt(inStr.c_str(),l,t,kStr.c_str(),kStr.size());
	    if (r < 0) {
	        srvInterface.log("AESEncrypt: Uhoh. Something bad happened during encryption.");
		//VT_THROW_EXCEPTION(r,"boom!");
		return;
	    } 

	    // Few debug lines...
	    //srvInterface.log("AESEncrypt: InLen: ->%d<-",l);
	    //srvInterface.log("AESEncrypt: Input: ->%s<-",inStr.c_str());
	    //srvInterface.log("AESEncrypt: Outpt: ->%s<-",t);
	    //srvInterface.log("AESEncrypt: OutLn: ->%d<-",r);
	    //srvInterface.log("AESEncrypt: StrLn: ->%d<-",(int)strlen(t));

	    res_writer.getStringRef().copy(t,r);
	    free(t);
	    res_writer.next();
	    
        } while (arg_reader.next());
    }
};

class AESEncryptFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, AESEncrypt); }

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
	// I chose to just round up to the next block size because
	// I believe it's faster than computing the length and round. 
        returnType.addVarchar(t.getStringLength()+AES_BLOCK_SIZE);
    }
};

RegisterFactory(AESEncryptFactory);

