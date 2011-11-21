/* 
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Transform Function: IISParserize a string
 *
 * Create Date: Nov 2, 2011
 */
#include "Vertica.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

/* There are 22 total possible fields in the w3c spec */
#define NUMFIELDS 22

using namespace Vertica;
using namespace std;

/*
 * Parse a standard w3c log file.
 */

// This is a list of all field names, in the same order as our output record
static const string incolumns[] = { "date", "time", "c-ip", "cs-username", "s-sitename", "s-computername",
                                    "s-ip", "s-port", "cs-method", "cs-uri-stem", "cs-uri-query", "sc-status",
                                    "sc-win32-status", "sc-bytes", "cs-bytes", "time-taken", "cs-version",
                                    "cs-host", "cs(User-Agent)", "cs(Cookie)", "cs(Referer)", "sc-substatus" };

// This is a list of all field names, as used by the output record
static const string outcolumns[] = { "date", "time", "c_ip", "cs_username", "s_sitename", "s_computername",
                                     "s_ip", "s_port", "cs_method", "cs_uri_stem", "cs_uri_query", "sc_status",
                                     "sc_win32_status", "sc_bytes", "cs_bytes", "time_taken", "cs_version",
                                     "cs_host", "cs_user_agent", "cs_cookie", "cs_referer", "sc_substatus" };
 
static bool loggingSet = false;

static void log(char *str) {
   if(loggingSet) {
      if(!str) {
         printf("INFO: NULL\n");
      } else {
         printf("INFO: %s\n", str);
      }
   }
}

static void log(string str) {
   log(str.c_str());
}

static void logValue(char *cstr, string str) {
   if(loggingSet) {
      if(str.empty()) {
         printf("VALU: [%s] [NULL]\n", cstr);
      } else {  
         printf("VALU: [%s] [%s]\n", cstr, str.c_str());
      }
   }
}

static void logValue(char *cstr, int val) {
   if(loggingSet) {
      string sval = "" + val;
      logValue(cstr, val);
   }
}

static void logFunction(char *cstr) {
   if(loggingSet) {
      printf("FUNC: %s\n", cstr);
   }
}

class w3cLogParser : public TransformFunction
{
   // When we get a #Fields: directive, this variable gets filled with the field list
   vector<string> fields;

   // Map for holding positions
   typedef std::map<string, int> FieldMap;

   // This is a mapping of column names (from incolumns) to record position in the output record
   FieldMap positions;

   // Offsets for the mapping
   vector<int> offsets;

   // fieldsSet is set to true after the #Fields: directive is received
   bool fieldsSet;

   virtual void processPartition(ServerInterface &srvInterface,
      PartitionReader &input_reader,
      PartitionWriter &output_writer)
   {
      try {

      // Until we get a Fields: directive, we can't process data rows
      logValue("fieldSet", "false");
      fieldsSet = false;
      
      // Pre-fill the positions map.  This maps the values in incolumns to values 0..n.
      // The mapping of incoming fields to positions is done in setFields()
      // e.g. incolumns[0] = 'date', so positions['date'] = 0.
      // This is really just for faster lookup...
      log("Pre-fill positions map");
      for (int i = 0; i < NUMFIELDS; i++) {
         positions[incolumns[i]] = i;
      }

      if (input_reader.getNumCols() != 1)
         vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

      // There will be one line for each line of data in the partition.  This function is called
      // exactly once per partition.  Because each log file is a partition, this function will be
      // called exactly once.  First will be some directives (#...), including the #Fields: directive.
      // 
      // All the other rows should be data rows.
      log("Begin processing rows");

      do
      {
         // The sentence variable contains the raw data (list of space-separated values)
         const VString &sentence = input_reader.getStringRef(0);
         std::string line = sentence.str();

         // Set all fields to null, as we may not have values for all of them
         log("Blanking all output fields");
         for(int i = 0; i < NUMFIELDS; i++)
         {
            VString &word = output_writer.getStringRef(i);
            word.setNull();
         }

         // If input string is NULL, then output is NULL as well.  
         if (sentence.isNull())
         {
	    // Don't do anything with this row
         }

         // If the line starts with an #, it is a directive.  Normally we will ignore these lines,
         // but the #Fields: directive is critical, as it tells us what fields are being sent,
         // and in what order.  Don't call output_writer, because there is no output row.
         else if (line.at(0) == '#') {
            logValue("Directive", line);

            if (line.find("#Fields:") == 0)
            { 
               log("Setting fields");
               setFields(line);
            }
            else log("Ignoring");
         }
         
         else {      
            log("Recieved a data row");
            // This is a data row.  Make sure we are ready for it!
            if(!fieldsSet) {
               stringstream ss;
               ss << "Received a data row, but the #Fields directive has not yet been received";
               throw ss.str();
            }
            // Parse the line into values (assumes space-delimited)
            vector<string> values;
            istringstream iss(line);
         
            do {
               string token;
               iss >> token;
               if(!token.empty()) {
                 values.push_back(token);
                 logValue("token", token);
               }
            } while (iss);

            // We should have exactly the same number of values as we have output fields
            if (values.size() != fields.size()) {
               stringstream ss;
               ss << "Input row has " << values.size() << " fields, expecting " << fields.size();
               throw ss.str();
            }
  
            // Now map the values into the correct placeholder in the result
            for (unsigned int i = 0; i < values.size(); i++)
            {
               // Get the field offset
               VString &word = output_writer.getStringRef(offsets[i]);

               // If the value is "-", then set the resulting column to null
               if( values[i].compare("-") == 0 )
               {
                  word.setNull();
               }
               // Set the appropriate output field to the found value
               else
               {
                  word.copy(values[i]);
               }
            }

            // Move to the next record
            output_writer.next();
         }
      } while (input_reader.next());
      
      }
      catch(string se) {
        vt_report_error(0, se.c_str());
      }
   }

   void setFields(const string &ref) { 
      logFunction("setFields");
      logValue("ref", ref);
      
      // Note which fields will now be parsed.  
      // Line looks like this:
      // #Fields: time cs-method cs-uri      

      // This is our list of fields.  It needs to be reset every time we see a #Fields: line      
      fields.clear();

      string input = ref;

      // Chop off the beginning and tokenize the rest
      input.erase(0, 9);

      istringstream iss(input);
      int i = 0;

      do {
         string token;
         iss >> token;

         if(!token.empty()) {
            fields.push_back(token);
            //printf("fields[%d]=%s\n", i++, token.c_str());
         }
      } while (iss);

      
      // Now that we have the field list, we need to create a map of incoming fields to positions
      // in the output record.
      log("Assign offset values");
      vector<string>::iterator it;
      offsets.clear();

      for ( it=fields.begin() ; it < fields.end(); it++ ) {
         map<string, int>::iterator mip;
         // Find the field name (*it) in the positions map
         mip = positions.find(*it);
        
         if(mip == positions.end()) {
            // Whoops, not in our list!  Throw big error!!!
            string val = *it;
            stringstream ss;
            ss << "This field is not recognized by the parser: " << val.c_str();
            throw ss.str();
         } else {
            offsets.push_back(mip->second);
         }
      }

      // Print out the offset mapping
      vector<int>::iterator vit;
      i = 0;
      for(vit=offsets.begin(); vit < offsets.end(); vit++) {
         //printf("offsets[%d]=%d\n", i++, *vit);
      }

      // Fields are set, ready to process data!
      logValue("fieldsSet", "true");
      fieldsSet = true;
   }
};

class w3cLogParserFactory : public TransformFunctionFactory
{
   // Tell Vertica that we take in a row with 1 string, and return a row with 15 strings
   virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
   {
      argTypes.addVarchar();

      returnType.addVarchar();  // Date
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();  // int
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();  // int
      returnType.addVarchar();
      returnType.addVarchar();  // int
      returnType.addVarchar();  // int
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
      returnType.addVarchar();
   }

   // Tell Vertica what our return string length will be, given the input
   // string length
   virtual void getReturnType(ServerInterface &srvInterface,
      const SizedColumnTypes &input_types,
      SizedColumnTypes &output_types)
   {
      // Error out if we're called with anything but 1 argument
      if (input_types.getColumnCount() != 1) 
         vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_types.getColumnCount());

      int input_len = 2048;

      // Our output size will never be more than the input size
      output_types.addVarchar(input_len, outcolumns[0]);  // date               
      output_types.addVarchar(input_len, outcolumns[1]);  
      output_types.addVarchar(input_len, outcolumns[2]); 
      output_types.addVarchar(input_len, outcolumns[3]);  
      output_types.addVarchar(input_len, outcolumns[4]);  
      output_types.addVarchar(input_len, outcolumns[5]);  
      output_types.addVarchar(input_len, outcolumns[6]);  
      output_types.addVarchar(input_len, outcolumns[7]);  // int                
      output_types.addVarchar(input_len, outcolumns[8]); 
      output_types.addVarchar(input_len, outcolumns[9]); 
      output_types.addVarchar(input_len, outcolumns[10]);
      output_types.addVarchar(input_len, outcolumns[11]); // int         
      output_types.addVarchar(input_len, outcolumns[12]); 
      output_types.addVarchar(input_len, outcolumns[13]); // int         
      output_types.addVarchar(input_len, outcolumns[14]); // int         
      output_types.addVarchar(input_len, outcolumns[15]); 
      output_types.addVarchar(input_len, outcolumns[16]); 
      output_types.addVarchar(input_len, outcolumns[17]); 
      output_types.addVarchar(input_len, outcolumns[18]); 
      output_types.addVarchar(input_len, outcolumns[19]); 
      output_types.addVarchar(input_len, outcolumns[20]); 
      output_types.addVarchar(input_len, outcolumns[21]); 
   }

   virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
      { return vt_createFuncObj(srvInterface.allocator, w3cLogParser); }


};

RegisterFactory(w3cLogParserFactory);
