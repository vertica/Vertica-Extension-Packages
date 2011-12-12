/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: Functions that help with full-text search over documents
 *
 * Create Date: Nov 25, 2011
 */
#include <sstream>
#include <set>
#include "Vertica.h"
#include "libstemmer.h"
#include <curl/curl.h>
#include <json/json.h>
#include "Words.h"
 
using namespace Vertica;
using namespace std;

/*
 * This is a simple function that stems an input word
 */
class Stem : public ScalarFunction
{
    sb_stemmer *stemmer;

public:
    // Set up the stemmer
    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        stemmer = sb_stemmer_new("english", NULL);
    }

    // Destroy the stemmer
    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        sb_stemmer_delete(stemmer);
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
        if (arg_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            std::string  inStr = arg_reader.getStringRef(0).str();

            const unsigned char* stemword = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(inStr.c_str()), inStr.size());

            // Copy string into results
            res_writer.getStringRef().copy(reinterpret_cast<const char*>(stemword));
            res_writer.next();
        } while (arg_reader.next());
    }
};

class StemFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, Stem); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
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
};

RegisterFactory(StemFactory);

// Fetches a document using CURL and tokenizes it
class DocTokenizer : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        if (input_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", input_reader.getNumCols());

        do {
            const VString &sentence = input_reader.getStringRef(0);

            // If input string is NULL, then output is NULL as well
            if (sentence.isNull())
            {
                VString &word = output_writer.getStringRef(0);
                word.setNull();
                output_writer.next();
            }
            else 
            {
                // Otherwise, let's tokenize the string and output the words
                std::string tmp = sentence.str();
                istringstream ss(tmp);

                do
                {
                    std::string buffer;
                    ss >> buffer;
                    
                    // Copy to output
                    if (!buffer.empty()) {
                        VString &word = output_writer.getStringRef(0);
                        word.copy(buffer);
                        output_writer.next();
                    }
                } while (ss);
            }
        } while (input_reader.next());
    }
};

class DocTokenFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 1 string
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();

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

        int input_len = input_types.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "words");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, DocTokenizer); }

};

RegisterFactory(DocTokenFactory);

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    stringstream *mem = (stringstream *)userp;

    mem->write(static_cast<const char*>(contents), realsize);
 
    return realsize;
}

static string fetch_url(CURL *curl_handle, const char *url)
{
    stringstream chunk;

    /* specify URL to get */ 
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
 
    /* send all data to this function  */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
 
    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
 
    /* get it! */ 
    curl_easy_perform(curl_handle);
            
    return chunk.str();
}

/*
 * Fetch the given URL as a string
 */
class FetchURL : public ScalarFunction
{
    CURL *curl_handle;

public:
    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        curl_global_init(CURL_GLOBAL_ALL);
 
        /* init the curl session */ 
        curl_handle = curl_easy_init();
    }

    // Destroy the stemmer
    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        /* cleanup curl stuff */ 
        curl_easy_cleanup(curl_handle);
        /* we're done with libcurl, so clean it up */ 
        curl_global_cleanup();
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
        if (arg_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        try {
            // While we have inputs to process
            do {
                // Get a copy of the input string
                std::string  inStr = arg_reader.getStringRef(0).str();

                /* fetch contents of URL */ 
                string contents = fetch_url(curl_handle, inStr.c_str()).substr(0, 65000);
 
                // Copy string into results
                res_writer.getStringRef().copy(contents);
                res_writer.next();
            } while (arg_reader.next());
        } catch (std::exception &e) {
            vt_report_error(0, e.what());
        }
    }
};

class FetchURLFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, FetchURL); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        returnType.addVarchar(65000); // max varchar size
    }
};

RegisterFactory(FetchURLFactory);


/*
 * Parse the Twitter JSON results
 */
class TwitterJSONParser : public TransformFunction
{
    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;

public:

    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input,
                                  PartitionWriter &output)
    {
        // Basic error checking
        if (input.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            input.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            string  inStr = input.getStringRef(0).str();

            bool success = reader.parse(inStr, root, false);
            if (!success)
                vt_report_error(0, "Malformed JSON text");
                
            // Get the query string and results
            string query = root.get("query", "-").asString();
            Json::Value results = root.get("results",  Json::Value::null);

            for (uint i=0; i<results.size(); i++) {
                Json::Value result = results[i];
                string text = result.get("text", "").asString();

                // Copy string into results
                output.getStringRef(0).copy(query);
                output.getStringRef(1).copy(text);
                output.next();
            }
        } while (input.next());
    }
};

class TwitterJSONFactory : public TransformFunctionFactory
{
    // return an instance of the transform function
    virtual TransformFunction *createTransformFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, TwitterJSONParser); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        returnType.addVarchar(100, "query"); // query string size <= 100
        returnType.addVarchar(300, "text"); // tweet size <= 300
    }
};

RegisterFactory(TwitterJSONFactory);


/*
 * Search Twitter and return results
 */
class TwitterSearch : public TransformFunction
{
    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;
    CURL *curl_handle;

public:
    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        curl_global_init(CURL_GLOBAL_ALL);
 
        /* init the curl session */ 
        curl_handle = curl_easy_init();
    }

    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        /* cleanup curl stuff */ 
        curl_easy_cleanup(curl_handle);
        /* we're done with libcurl, so clean it up */ 
        curl_global_cleanup();
    }

    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input,
                                  PartitionWriter &output)
    {
        // Basic error checking
        if (input.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            input.getNumCols());

        try {
            // While we have inputs to process
            do {
                // Get a copy of the input string
                string  inStr = input.getStringRef(0).str();

                // URL encode the search string
                char *sstr = curl_easy_escape(curl_handle, inStr.c_str(), 0);

                string url = "http://search.twitter.com/search.json?q=";
                url += sstr;

                while(true) {
                    string contents = fetch_url(curl_handle, url.c_str());

                    bool success = reader.parse(contents, root, false);
                    if (!success) {
                        srvInterface.log("Malformed JSON text: %s", contents.c_str());
                        break;
                    }
                    // Get the query string, next page and results
                    string query = root.get("query", "-").asString();
                    url = "http://search.twitter.com/search.json";
                    url += root.get("next_page", "").asString();

                    Json::Value results = root.get("results",  Json::Value::null);

                    if (results.size() == 0)
                        break;

                    for (uint i=0; i<results.size(); i++) {
                        Json::Value result = results[i];
                        string text = result.get("text", "").asString();

                        // Copy string into results
                        output.getStringRef(0).copy(query);
                        output.getStringRef(1).copy(text);
                        output.next();
                    }
                }

                curl_free(sstr);
            } while (input.next());
        } catch (std::exception &e) {
            vt_report_error(0, e.what());
        }
    }
};

class TwitterSearchFactory : public TransformFunctionFactory
{
    // return an instance of the transform function
    virtual TransformFunction *createTransformFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, TwitterSearch); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
        returnType.addVarchar();
    }

    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &argTypes,
                               SizedColumnTypes &returnType)
    {
        const VerticaType &t = argTypes.getColumnType(0);
        returnType.addVarchar(t.getStringLength() + 50, "query"); // to account for URL encoding
        returnType.addVarchar(300, "text"); // tweet size <= 300
    }
};

RegisterFactory(TwitterSearchFactory);

/*
 * This is a simple function that gives the sentiment score of a piece of
 * text. The score is simply the number of positive words minus the number of
 * negative words in the text.
 */
class Sentiment : public ScalarFunction
{
    set<string> positive;
    set<string> negative;

    sb_stemmer *stemmer;

    static inline void tokenize(const string &text, vector<string> &tokens)
    {
        istringstream ss(text);

        do {
            std::string buffer;
            ss >> buffer;

            if (!buffer.empty()) 
                tokens.push_back(buffer);
        } while (ss);
        
    }
public:
    // Set up the positive and negative word sets
    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        stemmer = sb_stemmer_new("english", NULL);

        int psize = sizeof(pwords)/sizeof(char*);
        int nsize = sizeof(nwords)/sizeof(char*);

        for (int i=0; i<psize; i++) {
            string stemword = reinterpret_cast<const char*>(sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(pwords[i]), strlen(pwords[i])));
            positive.insert(stemword);
        }

        for (int i=0; i<nsize; i++) {
            string stemword = reinterpret_cast<const char*>(sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(nwords[i]), strlen(nwords[i])));
            negative.insert(stemword);
        }
    }

    // Destroy the stemmer
    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) 
    {
        sb_stemmer_delete(stemmer);
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
        if (arg_reader.getNumCols() != 1)
            vt_report_error(0, "Function only accept 1 arguments, but %zu provided", 
                            arg_reader.getNumCols());

        // While we have inputs to process
        do {
            // Get a copy of the input string
            std::string  inStr = arg_reader.getStringRef(0).str();

            vector<string> words;
            tokenize(inStr, words);

            int score = 0;

            for (uint i=0; i<words.size(); i++) {
                string stemword = reinterpret_cast<const char*>(sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol*>(words[i].c_str()), words[i].size()));

                if (positive.count(stemword))
                    score++;
                if (negative.count(stemword))
                    score--;
            }

            // Copy score into results
            res_writer.setInt(score);
            res_writer.next();
        } while (arg_reader.next());
    }
};

class SentimentFactory : public ScalarFunctionFactory
{
    // return an instance of RemoveSpace to perform the actual addition.
    virtual ScalarFunction *createScalarFunction(ServerInterface &interface)
    { return vt_createFuncObj(interface.allocator, Sentiment); }

    virtual void getPrototype(ServerInterface &interface,
                              ColumnTypes &argTypes,
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addInt();
    }
};

RegisterFactory(SentimentFactory);
