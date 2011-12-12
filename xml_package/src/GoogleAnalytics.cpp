
/*
 *
 * Description: Example User Defined Transform Function: Transpose Function
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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>

#include <sstream>
#include <map>
#include <curl/curl.h>

#include "Vertica.h"
#include "VerticaProperties.h"
#include "Curl.h"
#include "Google.h"
#include "XMLFunctions.h"
#include "JulianDate.h"


using namespace Vertica;
using namespace std;



const int WIDTH = 2000;
const long unsigned int ARGS = 3;


class GAPIAanalyticsAuthorization : public ScalarFunction
{

public:
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
            	string auth = Google::authorize(arg_reader.getStringRef(0).str(), arg_reader.getStringRef(1).str(),
            			GOOGLE_SERVICE_ANALYTICS,
            			srvInterface);
                res_writer.getStringRef().copy(auth);
            }

            res_writer.next();
        } while (arg_reader.next());
    }
};


class GAPIAnalyticsGetAnalytics : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        do {
        	string authToken = Google::getAuthKey(input_reader.getStringRef(0).isNull()?string(""):input_reader.getStringRef(0).str(),
        			GOOGLE_SERVICE_ANALYTICS);

        	map<string,string> headers;
        	headers["Authorization"] = "GoogleLogin Auth="+authToken;
        	string tableID = input_reader.getStringRef(1).str();

	        srvInterface.log("COMPddLETRE\n");
	        srvInterface.log("COMPddLETRE\n");
	        srvInterface.log("COMPLxETRE\n");

        	JulianDate tempDate = JulianDate(input_reader.getDateRef(2));
        	string startDate = tempDate.toString();

        	tempDate = JulianDate(input_reader.getDateRef(3));
        	string endDate = tempDate.toString();

        	string url = string("https://www.google.com/analytics/feeds/data?ids=ga:")+
        			tableID+
        			string("&dimensions=ga:pagePath,ga:longitude,ga:latitude,ga:date,ga:hour&metrics=ga:visits,ga:pageviews&start-date=") +
        			startDate +
        			string("&end-date=")+
        			endDate +
        			string("&max-results=10000");

        	srvInterface.log("Date %lld %lld, URL: %s\n", input_reader.getDateRef(2),JulianDate(2009, 10, 10).getDateADT(), url.c_str());

          	string data = Curl::get(url,headers,srvInterface);
        	srvInterface.log("Response: %s\n",data.c_str());



        	if (((int) data.find("ErrorMsg"))>=0 || ((int) data.find("ERROR"))>=0 || ((int) data.find("Invalid"))>=0
        			|| ((int) data.find("precedes"))>=0 || ((int) data.find("permission"))>=0) {
        		output_writer.getStringRef(0).copy("ERROR, check authorization token: " + data);
				output_writer.next();
        	} else {
				string xslt =
					"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
					<xsl:stylesheet xmlns:atom=\"http://www.w3.org/2005/Atom\" \
						xmlns:dxp=\"http://schemas.google.com/analytics/2009\" \
						xmlns:gd=\"http://schemas.google.com/g/2005\" \
						xmlns:openSearch=\"http://a9.com/-/spec/opensearch/1.1/\"\
						xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\
						<xsl:output method=\"text\" version=\"1.0\" encoding=\"iso-8859-1\" \
							indent=\"no\"/>\
						<xsl:template match=\"/\">\
						  <xsl:for-each select=\"//atom:feed/atom:entry\">\
							  <xsl:for-each select=\"dxp:dimension\">\
								  <xsl:value-of select=\"@value\"/>\
								  <xsl:text>|</xsl:text>\
							  </xsl:for-each>\
							  <xsl:for-each select=\"dxp:metric\">\
								  <xsl:value-of select=\"@value\"/>\
								  <xsl:text>|</xsl:text>\
							  </xsl:for-each>\
							  <xsl:text>&#10;</xsl:text>\
						  </xsl:for-each>\
						</xsl:template>\
					 </xsl:stylesheet>";

				string response = XMLFunctions::xslt(data, xslt, srvInterface);

				SplitMap map = VerticaProperties::split(response,"|", "\n");

				for (int i=0; i<(int) map.size(); i++) {
					int intVal;

					output_writer.getStringRef(0).copy(map[intVal][0]);

					float f = atof(map[intVal][1].c_str());
					output_writer.getNumericRef(1).copy(f, true);   // latitude
					f = atof(map[intVal][2].c_str());
					output_writer.getNumericRef(2).copy(f, true);  // longitude

					sscanf(map[i][3].c_str(), "%d", &intVal);
					output_writer.setInt(3, intVal);  // hour

/* TODO: Fix
					string s = map[intVal][3];
		        	srvInterface.log("Response: %d %d %d %d\n",
							atoi(s.substr(0,4).c_str()),
							atoi(s.substr(4,2).c_str()),
							atoi(s.substr(6,2).c_str()),
							JulianDate(
														atoi(s.substr(0,4).c_str()),
														atoi(s.substr(4,2).c_str()),
														atoi(s.substr(6,2).c_str())).getDateADT()
		        			);


					output_writer.setDate(1,JulianDate(
							atoi(s.substr(0,4).c_str()),
							atoi(s.substr(4,2).c_str()),
							atoi(s.substr(6,2).c_str())).getDateADT());   // date
*/

					sscanf(map[i][4].c_str(), "%d", &intVal);
					output_writer.setInt(4, intVal);  // hour
					sscanf(map[intVal][5].c_str(), "%d", &intVal);
					output_writer.setInt(5, intVal); // visits
					sscanf(map[intVal][5].c_str(), "%d", &intVal);
					output_writer.setInt(5, intVal); // pageviews

					output_writer.next();
				}

        	}
        } while (input_reader.next());
    }

};

class GAPIAnalyticsGetTables : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        do {

        	string authToken = Google::getAuthKey(input_reader.getStringRef(0).isNull()?string(""):input_reader.getStringRef(0).str(),
        			GOOGLE_SERVICE_ANALYTICS);

        	map<string,string> headers;
        	headers["Authorization"] = "GoogleLogin Auth="+authToken;
        	string url = "https://www.google.com/analytics/feeds/accounts/default";

          	string data = Curl::get(url,headers,srvInterface);

          	if (data.size() == 0 || ((int) data.find("ErrorMsg"))>=0 || ((int) data.find("ERROR"))>=0 || ((int) data.find("permission"))>=0 ) {
        		output_writer.getStringRef(0).copy("ERROR, check authorization token: " + data);
				// output_writer.next();
        	} else {

				string xslt = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
					<xsl:stylesheet xmlns:atom=\"http://www.w3.org/2005/Atom\" \
						xmlns:dxp=\"http://schemas.google.com/analytics/2009\" \
						xmlns:gd=\"http://schemas.google.com/g/2005\" \
						xmlns:openSearch=\"http://a9.com/-/spec/opensearch/1.1/\"\
						xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\
						\
						<xsl:output method=\"text\" version=\"1.0\" encoding=\"iso-8859-1\" indent=\"no\"/>\
						<xsl:template match=\"/\">\
							<xsl:for-each select=\"//atom:entry\">\
								<xsl:value-of select=\"atom:id\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"atom:updated\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"atom:title\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:accountId']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:accountName']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:profileId']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:webPropertyId']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:currency']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:property[@name='ga:timezone']/@value\"/>\
								<xsl:text>|</xsl:text>\
								<xsl:value-of select=\"dxp:tableId\"/>\
								<xsl:text>&#10;</xsl:text>\
							</xsl:for-each>\
						</xsl:template>\
					</xsl:stylesheet>";


				string response = XMLFunctions::xslt(data, xslt, srvInterface);

				SplitMap map = VerticaProperties::split(response,"|", "\n");

				for (int i=0; i<(int) map.size(); i++) {
					output_writer.getStringRef(0).copy(map[i][0]); // account_url

					// TODO:

					//JulianDate d = JulianDate(2009,10,22);
					//output_writer.setDate(1,JulianDate(2009,10,22).getDateADT()); // Last update Date
					output_writer.setInt(1, atoi(map[i][1].c_str())); // Last update Date

					output_writer.getStringRef(2).copy(map[i][2]); // title
					output_writer.getStringRef(3).copy(map[i][3]); // account_id
					output_writer.getStringRef(4).copy(map[i][4]); // account_name
					output_writer.getStringRef(5).copy(map[i][5]); // profile_id
					output_writer.getStringRef(6).copy(map[i][6]); // web_property_id
					output_writer.getStringRef(7).copy(map[i][7]); // currency
					output_writer.getStringRef(8).copy(map[i][8]); // timezone
					output_writer.getStringRef(9).copy(map[i][9]); // table_id

					output_writer.next();
				}
        	}
        } while (input_reader.next());

    }

};

class GAPIAnalyticsAuthorizationFactory : public ScalarFunctionFactory
{
    // return an instance of GAuthorization to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, GAPIAanalyticsAuthorization); }

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
        returnType.addVarchar(2000);
    }
};



class GAPIAnalyticsGetAnalyticsFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
    	argTypes.addVarchar(); // Auth Token or Null
    	argTypes.addVarchar(); // tableID
    	argTypes.addDate();  // Start Date
    	argTypes.addDate();  // End Date
        returnType.addVarchar();
        returnType.addNumeric();
        returnType.addNumeric();
        // TODO: Fix date return type
//        returnType.addDate();
        returnType.addInt();

        returnType.addInt();
        returnType.addInt();
        returnType.addInt();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        output_types.addVarchar(4000, "pagePath");
        output_types.addNumeric(9,6, "longitude");
        output_types.addNumeric(9,6, "latitude");

        // TODO: Fix date return type
//        output_types.addDate("date");
        output_types.addInt("date");

        output_types.addInt("hour");
        output_types.addInt("visits");
        output_types.addInt("pageviews");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, GAPIAnalyticsGetAnalytics); }

};

class GAPIAnalyticsGetTablesFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
    	argTypes.addVarchar(); // Auth Token or Null
        returnType.addVarchar(); // account_url
        //TODO: FIx Date
        returnType.addInt(); // update_date
//        returnType.addDate(); // update_date
        returnType.addVarchar();  // title
        returnType.addVarchar();  // account_id
        returnType.addVarchar();  // account_name
        returnType.addVarchar();  // profile_id
        returnType.addVarchar();  // web_property_id
        returnType.addVarchar();  // currency
        returnType.addVarchar();  // timezone
        returnType.addVarchar();  // table_id
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        output_types.addVarchar(200, "account_url");
        // TODO: Fix Date
        output_types.addInt("update_date");
//        output_types.addDate("update_date");
        output_types.addVarchar(200, "title");
        output_types.addVarchar(200, "account_id");
        output_types.addVarchar(200, "account_name");
        output_types.addVarchar(200, "profile_id");
        output_types.addVarchar(200, "web_property_id");
        output_types.addVarchar(200, "currency");
        output_types.addVarchar(200, "timezone");
        output_types.addVarchar(200, "table_id");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, GAPIAnalyticsGetTables); }

};

RegisterFactory(GAPIAnalyticsAuthorizationFactory);
RegisterFactory(GAPIAnalyticsGetTablesFactory);
RegisterFactory(GAPIAnalyticsGetAnalyticsFactory);
// RegisterFactory(GLoginFactory);
