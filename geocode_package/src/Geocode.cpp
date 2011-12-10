/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Scalar Function: Remove spaces
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace Vertica;
using namespace std;


/*
 * This is a simple function that removes all spaces from the input string
 */
class Geocode : public ScalarFunction
{

public:
    CURL *curl;
    CURLcode res;
    std::string baseURL;

        /* callback to get latlong from curl response */
        size_t static
        WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
        {
          size_t realsize = size * nmemb;
          char * d = (char *)data;
          memcpy(d, ptr, realsize);
          d[realsize]=0; 
          return realsize;
        }


   virtual void setup(ServerInterface &srvInterface,
		      const SizedColumnTypes &argTypes) 
    {
       curl_global_init(CURL_GLOBAL_ALL);
       curl = curl_easy_init();
       baseURL = "https://webgis.usc.edu/Services/Geocode/WebService/GeocoderWebServiceHttpNonParsed_V02_96.aspx?apiKey=3ad26e5b22f144b0ad7e0fb2efc2c4b3&version=2.96&format=csv";
    }

    virtual void destroy(ServerInterface &srvInterface,
			 const SizedColumnTypes &argTypes)
    {
       curl_easy_cleanup(curl);
       curl_global_cleanup();
    }

    void split(vector<string> &result, string str, char delim ) {
      string tmp;
      string::iterator i;
      result.clear();

      for(i = str.begin(); i <= str.end(); ++i) {
        if((const char)*i != delim  && i != str.end()) {
          tmp += *i;
        } else {
          result.push_back(tmp);
          tmp = "";
        }
      }
    }

    
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
        if (arg_reader.getNumCols() != 4)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            arg_reader.getNumCols());
        
        // While we have inputs to process
        do {

            char curlresponse[1024]; 
            // Get a copy of the input string
            std::string  streetStr = arg_reader.getStringRef(0).str();
            std::string  cityStr = arg_reader.getStringRef(1).str();
            std::string  stateStr = arg_reader.getStringRef(2).str();
            std::string  zipStr = arg_reader.getStringRef(3).str();

            for (size_t pos = streetStr.find(' '); 
		 pos != string::npos;
		 pos = streetStr.find(' ',pos)) 
	    {
	      	streetStr.replace(pos,1,"%20");
	    } 

  	     for (size_t pos = cityStr.find(' ');
                 pos != string::npos;
                 pos = cityStr.find(' ',pos))                         
            {
                cityStr.replace(pos,1,"%20");
            }

            
            std::string URL = baseURL + "&streetAddress=" + streetStr;
            URL += "&city=" + cityStr;
            URL += "&state=" + stateStr;
            URL += "&zip=" + zipStr;

            if(curl) {
              curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
            }
    
            curl_easy_setopt(curl, CURLOPT_HEADER, 0);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlresponse );
	
            res = curl_easy_perform(curl);

            
	    // Copy string into results
            std::vector<std::string> tokens;
            split(tokens,string(curlresponse),',');

            std::string latlong = tokens[3] + "," + tokens[4];
            res_writer.getStringRef().copy(latlong);
            res_writer.next();
        } while (arg_reader.next());
    }
};

class GeocodeFactory : public ScalarFunctionFactory
{
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, Geocode); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
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
        returnType.addVarchar(1024);
    }
};

RegisterFactory(GeocodeFactory);
