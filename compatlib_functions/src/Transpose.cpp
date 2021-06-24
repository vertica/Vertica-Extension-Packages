
/*
 * Copyright (c) 2011 Vertica Systems, an HP Company
 *
 * Description: Example User Defined Transform Function: Transpose Function
 *
 * Create Date: June 21, 2011
 * Author: Patrick Toole
 */



/*


Function Signature: transpose (group_label, detail_label, separator)

STEP 1: COMPILE

g++ -D HAVE_LONG_LONG_INT_64 -I /opt/vertica/sdk/include -g -Wall -Wno-unused-value -shared -fPIC -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0 -o TransposeFunctions.so Transpose.cpp /opt/vertica/sdk/include/Vertica.cpp

STEP 2: LOAD INTO VERTICA

drop library TransposeFunctions cascade;
\set libfile '\''`pwd`'/TransposeFunctions.so\'';
CREATE LIBRARY TransposeFunctions AS :libfile;
create transform function transpose as language 'C++' name 'TransposeFactory' library TransposeFunctions;


create table people (id int, name varchar(20), gender varchar(1));
insert into people values (1, 'Patrick', 'M');
insert into people values (2, 'Jim', 'M');
insert into people values (3, 'Sandy', 'F');
insert into people values (4, 'Brian', 'M');
insert into people values (5, 'Linda', 'F');
commit;

select transpose(gender, name, ', ') over (partition by gender) from people;

 label |     group_label
-------+---------------------
 F     | Linda, Sandy
 M     | Jim, Brian, Patrick
(2 rows)




*/



#include "Vertica.h"
#include <sstream>
#include <map>

using namespace Vertica;
using namespace std;


const int WIDTH = 2000;
const long unsigned int ARGS = 3;

class Transpose : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
    	map<vint, vint> parent;
    	map<vint, string> label;
    	map<vint, string> separator;



    	if (input_reader.getNumCols() != ARGS)
            vt_report_error(0, "Function only accepts %d argument, but %zu provided", ARGS, input_reader.getNumCols());


    	string detail_list;
    	string group_name;

    	group_name = input_reader.getStringRef(0).str();
    	detail_list = input_reader.getStringRef(1).str();

    	while (input_reader.next()) {
    		//srvInterface.log("group: %s, name: %s", group_name.c_str(), input_reader.getStringRef(1).str().c_str());
        	detail_list = detail_list + input_reader.getStringRef(2).str() + input_reader.getStringRef(1).str() ;
        }

		VString &word0 = output_writer.getStringRef(0);
		word0.copy(group_name);

		VString &word1 = output_writer.getStringRef(1);
		word1.copy(detail_list);

		output_writer.next();
    }


};


class TransposeFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 5 inputs (parent ID, child ID, label, separator,
	// and return a row with 2 strings (id, path)
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();
        argTypes.addVarchar();

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
        if (input_types.getColumnCount() != ARGS)
            vt_report_error(0, "Function only accepts %d arguments, but %zu provided", ARGS, input_types.getColumnCount());


        // Our output size will never be more than the input size
        output_types.addVarchar(WIDTH, "label");
        output_types.addVarchar(WIDTH, "group_label");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, Transpose); }

};

RegisterFactory(TransposeFactory);

