/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/****************************
 * Vertica Analytic Database
 *
 * Example UDT to validate an email address
 *
 ****************************/
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <pcre.h>
#include "Vertica.h"
using namespace Vertica;


#define OVECCOUNT 30    /* should be a multiple of 3 */

using namespace std;


class EmailValidator : public TransformFunction
{
public:

    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
       
        if (input_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

        do {
            const VString &line = input_reader.getStringRef(0);
            std::string email = line.str();                        
            
            if (line.isNull()){
                VString &word = output_writer.getStringRef(0);
                word.copy("Invalid");
                output_writer.next();
                continue;
            }

            // Parse the line and output the validity of the email address
            checkEmailValidity(email, output_writer);

        }while(input_reader.next());
    }

private:  
        
    // Check and output the validity of the email address
    void checkEmailValidity(std::string email, PartitionWriter &output_writer ){

       const char *pattern = "[a-zA-Z0-9\\.\\+\\%\\-\\_]{1,}@[a-zA-Z0-9\\.\\-]{1,}\\.[a-zA-Z]{1,}";
       pcre *compiled_pattern;
       const char *error;
       int erroffset;
       int ovector[OVECCOUNT];
       int num_matches;
       int length = email.length();
       const char *email_str = email.c_str();

       VString &word = output_writer.getStringRef(0);
       
       //Compile the pattern to extract the name value pairs
       compiled_pattern =  pcre_compile(pattern, 0, &error, &erroffset, NULL);
              
       //Pattern compilation failed
       if (compiled_pattern == NULL){
           vt_report_error(0, "Email pattern compilation failed");
       }
       num_matches = pcre_exec(compiled_pattern, NULL, email_str, length, 0, 0, ovector,OVECCOUNT);

       if(num_matches < 0){
           //Invalid email address
           word.copy("Invalid");
           pcre_free(compiled_pattern);   
           output_writer.next();
       }         
       else{
           //Valid email address
           word.copy("Valid");
           pcre_free(compiled_pattern);           
           output_writer.next();
       }
       return;
    }

};


class EmailValidatorFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 1 string
    virtual void getPrototype(ServerInterface &srvInterface, 
                              ColumnTypes &argTypes, 
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
        
    }

    // Tell Vertica what our return string length will be,not more than the given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &inTypes, 
                               SizedColumnTypes &outTypes)
    {
                
        // Error out if we're called with anything but 1 argument
        if (inTypes.getColumnCount() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", inTypes.getColumnCount());

        int input_len = inTypes.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        outTypes.addVarchar(input_len, "Validity");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, EmailValidator); }

};

RegisterFactory(EmailValidatorFactory);
