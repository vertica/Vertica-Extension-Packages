/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

#include "Vertica.h"
#include "ContinuousUDParser.h"
#include "StringParsers.h"
#include "CRReader.hh"
#include "avro/Decoder.hh"
#include "avro/ValidSchema.hh"
#include "avro/Schema.hh"
#include "avro/Specific.hh"
#include "avro/Generic.hh"

using namespace Vertica;

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;

#include "AvroDatumParsers.h"

typedef std::vector<std::string> Strings;

class AvroParser : public ContinuousUDParser
{
public:
    AvroParser()
        { }
    
private:
    // Keep a copy of the information about each column.
    // Note that Vertica doesn't let us safely keep a reference to
    // the internal copy of this data structure that it shows us.
    // But keeping a copy is fine.
    SizedColumnTypes colInfo;


    // Source of parsed avro data
    CRReader<GenericDatum> * dataReader;
    avro::ValidSchema dataSchema;

    // Handles each field in the avro data, and writes it to the Vertica table
    bool handleField(const GenericDatum& d, size_t colNum)
        {
            return parseAvroFieldToType(d, colNum, colInfo.getColumnType(colNum), writer);
        }


public:
    
    virtual void initialize(ServerInterface &srvInterface, SizedColumnTypes &returnType)
        {
            colInfo = returnType;
        }

    virtual void run()
        {
            // Initialize the avro data reader to read according
            // to the source's schema
            dataReader = new CRReader<GenericDatum>(cr);
            GenericDatum datum(dataReader->dataSchema());
            
            int i = 1;
            // Iterate over rows in the avro file
            while (dataReader->read(datum))
            {

                // Fetch next row
                GenericRecord rec = datum.value<GenericRecord>();
                bool rejected;
                
                // Iterate over fields in the row
                for (size_t col = 0; col < rec.fieldCount(); col++ )
                {
                    GenericDatum d = rec.fieldAt(col);

                    // Parse the field according to the column's type,
                    // and write it to the table
                    rejected = !(handleField(d, col));

                    // Field could not be parsed according to the column's
                    // type, so the row won't be written
                    if ( rejected )
                        break;

                }

                // Write the row to the table if there were no problems
                if (!rejected)
                    writer->next();

                i++;
                
            }
            dataReader->close();
            delete dataReader;
            
        }
};

class AvroParserFactory : public ParserFactory
{
public:
    virtual void plan(ServerInterface &srvInterface,
                      PerColumnParamReader &perColumnParamReader,
                      PlanContext &planCtxt)
        {}
    
    virtual UDParser* prepare(ServerInterface &srvInterface,
                              PerColumnParamReader &perColumnParamReader,
                              PlanContext &planCtxt,
                              const SizedColumnTypes &returnType)
        {
            
            return vt_createFuncObj(srvInterface.allocator, AvroParser);
            
        }
    
    virtual void getParserReturnType(ServerInterface &srvInterface,
                                     PerColumnParamReader &perColumnParamReader,
                                     PlanContext &planCtxt,
                                     const SizedColumnTypes &colTypes,
                                     SizedColumnTypes &returnType) 
        {
            returnType = colTypes;
        }
    
    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes)
        {
        }
    
};
RegisterFactory(AvroParserFactory);
