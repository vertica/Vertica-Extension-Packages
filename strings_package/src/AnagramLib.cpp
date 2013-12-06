#include "Vertica.h"
#include <string.h>

using namespace Vertica;

// Anagram generator
class AnagramGenerator : public TransformFunction
{
    void recurse(PartitionWriter &output_writer, char *wd, int l, int p)
    {
        // Emit the prefix of the word
        if (p != 0) {
            char tmp = wd[p];
            wd[p] = 0; // terminate
            output_writer.getStringRef(0).copy(wd);
            output_writer.next();
            wd[p] = tmp; // restore
        }

        // Stop recursing when whole word has been tried
        if (p == l) return;

        // Swap all remaining letters in as next letter
        for (int np = p; np<l; ++np) {
            // swap
            char tmp = wd[p];
            wd[p] = wd[np];
            wd[np] = tmp;

            // recurse
            recurse(output_writer, wd, l, p+1);

            // swap back
            wd[np] = wd[p];
            wd[p] = tmp;
        }

        // anagramarama!
    }

    virtual void processPartition(ServerInterface &srvInterface, PartitionReader &input_reader, PartitionWriter &output_writer)
    {
          // Get our string into something easily messed with
          const VString &wd = input_reader.getStringRef(0);
          if (wd.isNull()) return;
          int l = wd.length();
          char word[l+1];
          memset(word, 0, l+1);
          memcpy(word, wd.data(), l);

          // Do the anagrams
          recurse(output_writer, word, l, 0);
    }
};

class AnagramFactory : public TransformFunctionFactory
{
    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, AnagramGenerator); }

    virtual void getReturnType(ServerInterface &srvInterface, const SizedColumnTypes &input_types, SizedColumnTypes &output_types)
    {
        // Length matches input
        output_types.addArg(input_types.getColumnType(0), "anagram");
    }

    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addVarchar();
    }
};

RegisterFactory(AnagramFactory);
