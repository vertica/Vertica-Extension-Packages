/* Copyright (c) 2005 - 2011 Vertica, an HP company -*- C++ -*- */
/* 
 *
 * Description: User Defined Transform Function that invokes its input argument as a shell command
 *
 * Create Date: Apr 29, 2011
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>               // getrlimit
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include <algorithm>

#include "Vertica.h"
#include "ProcessLaunchingPlugin.h"

using namespace Vertica;

#define LINE_MAX 65000

typedef unsigned char byte;

/*
 * This function invokes each input string command as if it was run with: bash -c '<command>'
 * The output is split on newlines and output as rows.
 */
class Shell : public TransformFunction
{
private:
  // Can't be too large in case we're ever stack-allocated
  // But this should fit on any reasonable stack
  byte input_buf[256 * 1024];
  byte output_buf[256 * 1024];
  vint line_num;

  sighandler_t orig_handler;
  
  bool fillInput(DataBuffer &input, size_t input_max_size, PartitionReader &input_reader, int col, bool keepExistingData = true) {
    // First, keep any existing data
    // memmove() handles the case of overlapping data

    if (keepExistingData) {
      memmove(input.buf, input.buf + input.offset, input.size - input.offset);
      input.size = input.size - input.offset;
    } else {
      input.size = 0;
    }

    input.offset = 0;

    const VString *str = input_reader.getStringPtr(col);
    while (str->isNull() || str->length() <= input_max_size - input.size) {
      if (!str->isNull()) {
	memcpy(input.buf + input.offset, str->data(), str->length());
	input.size += str->length();
      }
      if (!input_reader.next()) return false;
      str = input_reader.getStringPtr(col);
    }

    return true;
  }

  void emitOutput(DataBuffer &output, PartitionWriter &output_writer, bool flush_output = false) {
    printf("Output buffer: %zu of %zu bytes used [%s]\n", output.offset, output.size, output.buf);
    fflush(stdout);
    size_t pos = 0;
    size_t line_start = 0;
    while (pos < output.offset) {
      if (output.buf[pos] == '\n') {
	++pos;

	// Hardcode which data goes into which column
	output_writer.setInt(0, (vint)line_num++);
	output_writer.getStringRef(1).copy((const char*)&output_buf[line_start], std::min((vint)(pos - line_start), (vint)65000));
	output_writer.next();

	line_start = pos;
      } else {
	++pos;
      }
    }

    if (flush_output) {
      // Hardcode which data goes into which column
      output_writer.setInt(0, (vint)line_num++);
      output_writer.getStringRef(1).copy((const char*)&output_buf[line_start], std::min((vint)(pos - line_start), (vint)65000));
      output_writer.next();
      output.offset = 0;
    } else {
      // Keep unused data in the buffer for next time
      memmove(output.buf, output.buf + pos, output.offset - pos);
      output.offset = pos;
    }
  }

public:

    virtual void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes)
    {
        orig_handler = signal(SIGCHLD, SIG_DFL);
    }

    virtual void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes)
    {
        // Totally not thread-safe.  Oh well...
        if (orig_handler != SIG_DFL) signal(SIGCHLD, orig_handler);
    }

    /*
     * For each partition, executes each string input by forking an invocation
     * of bash.  The resulting output is broken up by newlines and becomes the
     * output rows.
     */
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
      printf("test\n");
      fflush(stdout);
      // Emulate an ExternalFilter
      DataBuffer input;
      input.buf = (char*)&input_buf[0];
      input.size = sizeof(input_buf);
      input.offset = 0;
      // Fetch data for the initial input buffer
      fillInput(input, sizeof(input_buf), input_reader, 0, false /* keepExistingData */);

      DataBuffer output;
      output.buf = (char*)&output_buf[0];
      output.size = sizeof(output_buf);
      output.offset = 0;
      // Initial output buffer should be empty; no data to fetch

      StreamState state;
      InputState input_state = OK;

      ProcessLaunchingPlugin p(srvInterface.getParamReader().getStringRef("cmd").str(),
			       std::vector<std::string>());

      p.setupProcess();
      
      do {
	state = p.pump(input, input_state, output);

	switch (state) {
	case INPUT_NEEDED: 
	  if (input_state != END_OF_FILE && input_reader.getNumRows() > 0) {
	    input_state = (fillInput(input, sizeof(input_buf), input_reader, 0)
			   ? OK : END_OF_FILE);
	  }
	  break;	  
	case OUTPUT_NEEDED:
	  emitOutput(output, output_writer);
	  break;
	case DONE:
	  break;
	case KEEP_GOING:
	  continue;
	default:
	  vt_report_error(0, "Unsupported StreamState: %d", (int)state);
	}
      } while (state != DONE);

      emitOutput(output, output_writer, true /* flush_output */);

      p.destroyProcess();
    }
};

class ShellFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 2 strings
    virtual void getPrototype(ServerInterface &srvInterface, 
                              ColumnTypes &argTypes, 
                              ColumnTypes &returnType)
    {
        argTypes.addVarchar();

	returnType.addInt();
        returnType.addVarchar();
    }

    virtual void getParameterType(ServerInterface &srvInterface,
				  SizedColumnTypes &parameterTypes) {
      parameterTypes.addVarchar(65000, "cmd");
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types)
    {
        // Error out if we're called with anything but 1 argument
        if (input_types.getColumnCount() != 1)
            vt_report_error(0, "Function only accepts 1 argument, but %zu provided", 
                            input_types.getColumnCount());

	output_types.addInt("line_num");

        // other output is a line of output from the shell command, which is
        // truncated at LINE_MAX characters
        output_types.addVarchar(LINE_MAX, "text");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, Shell); }

};

RegisterFactory(ShellFactory);

