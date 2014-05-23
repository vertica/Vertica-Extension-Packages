/* Copyright (c) 2014 Peakgames -*- C++ -*- */
/* 
 *
 * Description: Extract value from JSON according to specified JsonPath
 *
 */
#include "Vertica.h"
#include "json/json.h"

using namespace Vertica;

/*
 * JSON_EXTRACT_PATH_TEXT('json_string', 'path_elem')
 */
class JsonExtractPathText : public ScalarFunction
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
			vt_report_error(0, "Function accepts 2 arguments, but %zu provided",
							arg_reader.getNumCols());

		// While we have inputs to process
		do {
			// Get a copy of the input string
			std::string inStr = arg_reader.getStringRef(0).str();
			std::string pathStr = arg_reader.getStringRef(1).str();

			Json::Value root;
			Json::Reader json_reader;
			Json::FastWriter json_writer;

			if (! json_reader.parse(inStr, root) ) {
				res_writer.getStringRef().setNull(); // Failed to parse, return NULL
			} else {
				Json::Path xpath(pathStr, Json::PathArgument(), Json::PathArgument(), Json::PathArgument(), Json::PathArgument(), Json::PathArgument());
				root = xpath.resolve(root);

				if (root.isNull()) {
					res_writer.getStringRef().setNull(); // NULL is NULL
				} else {
					std::string val;

					if (root.isString()) {
						val = root.asString(); // Get string
					} else {
						val = json_writer.write(root); // Get JSON value
						val.erase(val.end() - 1); // Remove trailing newline from FastWriter::write
					}

					res_writer.getStringRef().copy(val);
				}
			}

			res_writer.next();
		} while (arg_reader.next());
	}
};

class JsonExtractPathTextFactory : public ScalarFunctionFactory
{
	virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
	{ return vt_createFuncObj(interface.allocator, JsonExtractPathText); }

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

public:
	JsonExtractPathTextFactory()
	{
		vol = IMMUTABLE;
		strict = RETURN_NULL_ON_NULL_INPUT;
	}

};

RegisterFactory(JsonExtractPathTextFactory);
