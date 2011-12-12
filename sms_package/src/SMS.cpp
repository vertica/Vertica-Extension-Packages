/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: User Defined Scalar Function: SMS, for sending a short txt message
 *
 * Create Date: Dec 12, 2011
 */
#include <stdio.h>
#include <curl/curl.h>

#include "Vertica.h"

using namespace Vertica;

/*
 * Send a short txt message via web service
 */
class SMS : public ScalarFunction
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
        if (arg_reader.getNumCols() != 5)
            vt_report_error(0, "Function only accept 5 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            const std::string& sid = arg_reader.getStringRef(0).str();
            const std::string& auth = arg_reader.getStringRef(1).str();
            const std::string& from_num = arg_reader.getStringRef(2).str();
            const std::string& to_num = arg_reader.getStringRef(3).str();
            const std::string& body = arg_reader.getStringRef(4).str();

	    std::string httpOutput;
	    int ret = sendSms(sid.c_str(), 
	    		      auth.c_str(),
	    		      from_num.c_str(),
	    		      to_num.c_str(),
	    		      body.c_str(),
                              httpOutput);

	    srvInterface.log("SMS call return: %d", ret);
	    srvInterface.log("SMS web service output: %s", httpOutput.c_str());

	    std::string retMsg;
	    if (ret != 0)
	      retMsg = "Message was not sent due to an error.";
	    else
            {
	      retMsg = "Successfully invoked SMS web service with message to " + to_num + " and content: " + body;
              retMsg += "\n\n Web service output:\n" + httpOutput;
            }

            // Copy string into results
            res_writer.getStringRef().copy(retMsg.c_str());
            res_writer.next();
        } while (arg_reader.next());
    }

private:

  int sendSms(const char* sid,
	      const char* auth,
	      const char* from_num,
	      const char* to_num,
	      const char* body,
	      std::string& httpOutput)
  {
    CURL *curl;
    CURLcode res;

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
 
    curl_global_init(CURL_GLOBAL_ALL);
 
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "From",
                 CURLFORM_COPYCONTENTS, from_num,
                 CURLFORM_END);
 
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "To",
                 CURLFORM_COPYCONTENTS, to_num,
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "Body",
                 CURLFORM_COPYCONTENTS, body,
                 CURLFORM_END);

    curl = curl_easy_init();
    if (!curl) {
      httpOutput = "Cannot initialize curl";
      return -1;
    }

    char url[4096];
    sprintf(url, "https://%s:%s@api.twilio.com/2010-04-01/Accounts/%s/SMS/Messages", sid, auth, sid);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SMS::writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpOutput);
    res = curl_easy_perform(curl);
    
    if (res != 0) 
      httpOutput = curl_easy_strerror(res);

    /* always cleanup */ 
    curl_easy_cleanup(curl);
    return res;
  }

  static size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *s)
  {
    *s += std::string((const char*)ptr);

    return size*nmemb;
  }


};

class SMSFactory : public ScalarFunctionFactory
{
    // return an instance of SMS to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, SMS); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        argTypes.addVarchar();
        argTypes.addVarchar();
        argTypes.addVarchar();

        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        returnType.addVarchar(65000);
    }
};

RegisterFactory(SMSFactory);

