/* Copyright (c) 2011 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: UDT to compute Anagrams
 *
 * Create Date: Apr 29, 2011
 */
#include "Vertica.h"
#include <list>
#include <sstream>

using namespace Vertica;
using namespace std;


/**
 * Compute all NGrams of words in a sentence: all contiguous N word sequences
 * in a sentence.
 *
 * http://en.wikipedia.org/wiki/N-gram
 */
class NGramFunction : public TransformFunction
{
public:
    NGramFunction(int theN) : _N(theN) {}

    // What is the N?
    size_t _N;

    // concatenate all words in queue into a single space delimited string
    //
    // input: <hello> <world>
    // output: "hello world"
    static void outputQueue(PartitionWriter &output_writer,
                            const list<string> &words)
    {
        // cheat with an extra copy into oss rather that writing to storage
        // directly (sorry chuck)
        ostringstream oss;
        list<string>::const_iterator it;
        for (it = words.begin(); it != words.end(); ++it) 
        {
            // Spaces after first one
            if (it != words.begin()) oss << " ";
            oss << *it;
        }
        output_writer.getStringRef(0).copy(oss.str());
        output_writer.next();
    }

    void computeNGrams(PartitionWriter &output_writer,
                       const string &sentence) 
    {
        // keep track of the last N words
        list<string> lastN;
        
        // Process each input word at a time.
        istringstream ss(sentence);
        do
        {
            std::string t;
            ss >> t;

            // Ignore empty input strings
            if (!t.empty()) {
                lastN.push_back(t);
                
                // If we have N items, output it!
                if (lastN.size() == _N) {
                    outputQueue(output_writer, lastN);
                    lastN.pop_front(); 
                }
            }


        } while (ss);
    }

    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer)
    {
        do {
            if (input_reader.getStringRef(0).isNull()) 
            {
                output_writer.getStringRef(0).setNull();
            } 
            else 
            {
                string inputString = input_reader.getStringRef(0).str();
                computeNGrams(output_writer, inputString);
            } 
        } while (input_reader.next());
    }
};


/*
 * Boilerplate
 */
class NGramFactory : public TransformFunctionFactory
{
    // Provide the function prototype information to the Vertica server (argument types + return types)
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
    }

    virtual const char *getName() = 0;
    virtual int         getN()    = 0;

    // Provide return type length/scale/precision information (given the input
    // type length/scale/precision), as well as column names
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        output_types.addArg(input_types.getColumnType(0), getName());
    }

    // Create an instance of the TransformFunction
    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { 
        return vt_createFuncObj(srvInterface.allocator, NGramFunction, getN()); 
    }

};


class TwoGramsFactory : public NGramFactory {
    const char *getName() { return "twogram"; }
    int getN()            { return 2;         }
};
RegisterFactory(TwoGramsFactory);

class ThreeGramsFactory : public NGramFactory {
    const char *getName() { return "threegram"; }
    int getN()            { return 3;         }
};
RegisterFactory(ThreeGramsFactory);

class FourGramsFactory : public NGramFactory {
    const char *getName() { return "fourgram"; }
    int getN()            { return 4;         }
};
RegisterFactory(FourGramsFactory);

class FiveGramsFactory : public NGramFactory {
    const char *getName() { return "fivegram"; }
    int getN()            { return 5;         }
};
RegisterFactory(FiveGramsFactory);
