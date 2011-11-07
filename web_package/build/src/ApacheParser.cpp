/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/****************************
 * Vertica Analytic Database
 *
 * Example UDT to parse apache log files
 *
 ****************************/
#include "Vertica.h"

using namespace Vertica;

#include <fstream>
#include <sstream>

#include <string>
#include <vector>


using namespace std;


// Parses a fixed list of delimiters from a string
struct DelimiterParser
{
    DelimiterParser() : _idx(0), _delimIdx(0) {}

    // Describes a field: delimiter and max length
    struct DelimDesc
    {
        DelimDesc(const string &n, const string &delim, size_t len) : 
            _name(n), _delim(delim), _len(len) { _fieldNames.push_back(n); }

        // Specify that the delimiter represents several fields with the specified names
        void setFields(const std::vector<string> &fieldNames) { _fieldNames = fieldNames; }
        
        // Get the name of the idx'th field
        const std::string& getFieldName(size_t idx) { return _fieldNames.at(idx); }

        size_t getNumFields()               const { return _fieldNames.size(); }
        size_t getLen()                     const { return _len;       }
        const std::string& getDelimString() const { return _delim;     }        

    private:
        string         _name;
        string         _delim;
        size_t         _len;
        vector<string> _fieldNames; // maximum number of fields (split on space)
    };

    // Add a delimiter to this parser
    DelimDesc& addDelim(const string &dname, const string &delim, size_t len = 64000) 
    {
        _delims.push_back(DelimDesc(dname, delim, len));
        return _delims.back();
    }

    // iterator structure over delimiters and fields
    struct iterator
    {
        iterator(DelimiterParser *parser, size_t idx=0) : 
            _parser(parser), _delimIdx(idx), _fieldIdx(0) {}

        // postfix ++ operator to advance to next field
        iterator &operator++(int) 
        { 
            if (_fieldIdx == d().getNumFields()-1) {
                _delimIdx++; _fieldIdx = 0; 
            } else {
                _fieldIdx++;
            }
            return *this;
        }

        bool operator!=(const iterator &rhs) 
        { 
            return _delimIdx != rhs._delimIdx || _fieldIdx != rhs._fieldIdx;
        }

        bool        onFirstField() { return _fieldIdx == 0; }
        std::string getFieldName() { return d().getFieldName(_fieldIdx); }
        size_t      getFieldSize() { return d().getLen();                }
        int         getNumFields() { return d().getNumFields();          }

    private:
        // return the current delimiter we are working at
        DelimDesc &d() { return _parser->getDelim(_delimIdx); }

        DelimiterParser *_parser;
        size_t           _delimIdx; // index into _delims
        size_t           _fieldIdx; // current field of current delim
    };
        
    DelimDesc &getDelim(size_t idx) { return _delims.at(idx); }

    // Standard stl like iterator interface
    iterator begin() { return iterator(this); }
    iterator end()   { return iterator(this, _delims.size()); }

    // Gets the next field
    string parseNext();

    // resets for next parsing line
    void nextLine(const string &line) { _input = line; _idx = 0; _delimIdx = 0; }

private:

    string            _input;
    size_t            _idx; // current parse location

    vector<DelimDesc> _delims;
    size_t            _delimIdx;
};

// Parse the next delimited field from input starting at idx. Updates idx
string DelimiterParser::parseNext()
{
    const string &delim = getDelim(_delimIdx++).getDelimString();

    // empty delim means rest of string
    if (delim.empty()) return _input.substr(_idx);
    
    // Get the next field
    size_t next = _input.find(delim, _idx);


    const size_t maxIdx = _input.size();

    // If not found, take rest of the string
    if (next == string::npos) next = maxIdx;

    string ret = _input.substr(_idx, next-_idx);

    // remember where we are next time
    _idx = next + delim.size();
    // ensure we don't go off the end of the string
    if (_idx > maxIdx) _idx = maxIdx;
    return ret;
}

// Specialization of parser for apache log files
struct ApacheDelimiterParser : public DelimiterParser
{
    ApacheDelimiterParser();

    // Parse the next line into the appropriate fields of the output writer.
    void parseLine(const string &line, PartitionWriter &output_writer);

    // Generates the appropriate line if the input was null
    void parseNullLine(PartitionWriter &output_writer);

    // Adds the output fields this parser creates to types
    void addFields(ColumnTypes &types);

    // Adds the appropriately sized fields this parser creates 
    void addFields(SizedColumnTypes &types, size_t inputLen);
};

ApacheDelimiterParser::ApacheDelimiterParser()
{
    // We are parsing lines lines like this
    //
    // 184.247.8.233 - - [05/May/2011:08:52:25 -0700] "GET /pictures/C/CW1/ HTTP/1.1" 401 528 "-" "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_5_7; en-us) AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Safari/530.17"

    // Note the delimiters are for the end of the field
        
    addDelim("ip", " ", 1000);
    addDelim("unknown", " ", 1000);
    addDelim("username", " [", 1000);
    // TODO: parse natively as a timestamp rather than postprocess    
    addDelim("ts_str",   "] \"", 1000);
    // combines type and URL. Examples:
    // 
    // "GET /xmas_tree/113-1335_IMG.JPG HTTP/1.1"
    // "GET /xmas_tree/113-1335_IMG.JPG"
    vector<string> fields; 
    fields.push_back("request_type"); 
    fields.push_back("request_url"); 
    fields.push_back("request_version");
    addDelim("raw_request", "\" ").setFields(fields);
    addDelim("response_code", " ", 1000);
    addDelim("response_size", " \"", 1000);
    addDelim("referring_url", "\" ");
    addDelim("user_agent", "");
}

void ApacheDelimiterParser::parseLine(const string &line, 
                                      PartitionWriter &output_writer)
{
    nextLine(line);

    size_t i = 0;
    for (iterator it = begin(); it != end(); it++) {
        // skip if not the first field (the field advance is done in the loop below)
        if (!it.onFirstField()) continue;

        // Parse out the next field from the delimiter
        string fldVal = parseNext();

        // Special case no value and '-' into NULLs for the DB
        if (fldVal.empty() || fldVal == "-")
        {
            for (int j=0; j<it.getNumFields(); j++) {
                output_writer.getStringRef(i++).setNull();
            }
        }
        else 
        {
            size_t pos = 0;
            for (int j=0; j<it.getNumFields()-1; j++)  
            {
                VString &res = output_writer.getStringRef(i++);

                size_t idx = fldVal.find(' ', pos);
                if (idx == string::npos) 
                {
                    if (pos == fldVal.size()) res.setNull(); // already at end of string
                    else                      res.copy(fldVal.substr(pos)); // take rest of string
                    pos = fldVal.size();
                } 
                else 
                {
                    res.copy(fldVal.substr(pos, idx-pos));
                    pos = idx+1;
                }
            }
            // last field is entire remaining string
            if (pos == fldVal.size())
              output_writer.getStringRef(i++).setNull();
            else 
              output_writer.getStringRef(i++).copy(fldVal.substr(pos));
        }
    }
}

void ApacheDelimiterParser::parseNullLine(PartitionWriter &output_writer)
{
    size_t i = 0;
    for (iterator it = begin(); it != end(); it++, i++) {
        output_writer.getStringRef(i).setNull();
    }
}


void ApacheDelimiterParser::addFields(ColumnTypes &types)
{
    for (iterator it = begin(); it != end(); it++) {
        types.addVarchar(); // TODO: more generic types
    }
}

// Adds the appropriately sized fields this parser creates 
void ApacheDelimiterParser::addFields(SizedColumnTypes &types, size_t inputLen)
{
    for (iterator it = begin(); it != end(); it++) {
        size_t fldLen = it.getFieldSize();
        // output can be at most the size of the input
        if (fldLen > inputLen) fldLen = inputLen;
        types.addVarchar(fldLen, it.getFieldName());
    }
}


class ApacheParser : public TransformFunction
{
public:
    ApacheParser() {}

    // All parsing state and logic
    ApacheDelimiterParser _parser;

    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        do {
            const VString &line = input_reader.getStringRef(0);

            // If input string is NULL, then output is NULL as well
            if (line.isNull())
            {
                _parser.parseNullLine(output_writer);
            }
            else 
            {
                _parser.parseLine(line.str(), output_writer);
            }

            output_writer.next();
        } while (input_reader.next());
    }
};

class ApacheParserFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 1 string
    virtual void getPrototype(ServerInterface &srvInterface, 
                              ColumnTypes &argTypes, 
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        ApacheDelimiterParser parser;
        parser.addFields(returnType);
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &inTypes, 
                               SizedColumnTypes &outTypes)
    {
        size_t inputLen = inTypes.getColumnType(0).getStringLength();
        ApacheDelimiterParser parser;
        parser.addFields(outTypes, inputLen);
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, ApacheParser); }

};

RegisterFactory(ApacheParserFactory);
