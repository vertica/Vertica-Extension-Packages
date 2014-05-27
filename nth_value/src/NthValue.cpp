/* Copyright (c) 2014 Peakgames -*- C++ -*- */
/* 
 *
 * Description: NTH_VALUE Analytic Function
 *
 */
#include "Vertica.h"

using namespace Vertica;

/*
 * NTH_VALUE(expr, offset) OVER (...)
 */
class NthValue : public AnalyticFunction
{
public:

	virtual void processPartition(ServerInterface &srvInterface,
			AnalyticPartitionReader &arg_reader,
			AnalyticPartitionWriter &res_writer)
	{
		// Basic error checking
		if (arg_reader.getNumCols() != 4)
			vt_report_error(0, "Function accepts 2 arguments, but %zu provided",
							arg_reader.getNumCols() - 2);

		vint count = 0;
		vint isFound = 0;
		std::string ourVal;

		// While we have inputs to process
		do {
			// Get a copy of the input expression
			std::string expr = arg_reader.getStringRef(2).str();
			const vint offset = arg_reader.getIntRef(3);

			if (offset < 1) {
				vt_report_error(0, "This function only accepts positive integers as offset");
				}

			count++; // our row number for this partition

			if (count == offset) {
				ourVal = expr; // this is our return value for this partition
				isFound = 1;
				for(vint i=0;i<count;i++) {
					res_writer.getStringRef(0).copy(ourVal); // set all previous rows (including current) to our return value
					res_writer.next();
					}
			} else if (isFound == 1) {
				res_writer.getStringRef(0).copy(ourVal); // set all next rows to our return value
				res_writer.next();
				}
		} while (arg_reader.next());

		if (isFound == 0) { // offset not found, return NULL for all rows
			for(vint i=0;i<count;i++) {
				res_writer.getStringRef(0).setNull();
				res_writer.next();
				}
		}

	}
};

class NthValueFactory : public AnalyticFunctionFactory
{
	virtual AnalyticFunction *createAnalyticFunction(ServerInterface &interface)
	{ return vt_createFuncObj(interface.allocator, NthValue); }

	virtual void getPrototype(ServerInterface &interface,
							ColumnTypes &argTypes,
							ColumnTypes &returnType)
	{
		argTypes.addVarchar();
		argTypes.addInt();
		returnType.addVarchar();
	}

	virtual void getReturnType(ServerInterface &srvInterface,
							const SizedColumnTypes &argTypes,
							SizedColumnTypes &returnType)
	{
		const VerticaType &t = argTypes.getColumnType(2);
		returnType.addVarchar(t.getStringLength());
	}

};

RegisterFactory(NthValueFactory);
