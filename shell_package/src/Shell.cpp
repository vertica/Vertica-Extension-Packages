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

#include "Vertica.h"

using namespace Vertica;

#define SHELL_EXECUTABLE "/bin/bash"
#define BUF_SIZE 1024
#define LINE_MAX 64000

/*
 * This function invokes each input string command as if it was run with: bash -c '<command>'
 * The output is split on newlines and output as rows.
 */
class Shell : public TransformFunction
{
public:

    /**
     * ensures don't leave any loose pipes open on exit
     */
    struct ScopedPipes {
        int pipes[2];

        ScopedPipes()
        { 
            pipes[0] = 0; 
            pipes[1] = 0;

            if (pipe(pipes) < 0) {
                vt_report_error(0, "Unable to open pipe");
            }
        }

        void closePipe(int i)
        {
            if (pipes[i])
            {
                close(pipes[i]);
                pipes[i] = 0;
            }
        }

        ~ScopedPipes()
        {
            closePipe(0);
            closePipe(1);
        }
    };

    /**
     * ensures don't leave any loose sub processes lying around on exit
     */
    struct ScopedSubprocess {
        pid_t pid;

        ScopedSubprocess(pid_t p) : pid(p) {}
        
        void terminate(bool gently)
        {
            //if (!gently) kill(pid,SIGKILL);
            int result=0;
            waitpid(pid, &result, 0);
            pid = 0;
        }
        
        ~ScopedSubprocess()
        {
            terminate(false/*by kill!*/);
        }
    };

    /*
     * For each partition, executes each string input by forking an invocation
     * of bash.  The resulting output is broken up by newlines and becomes the
     * output rows.
     */
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        // Basic error checking
        if (input_reader.getNumCols() != 2)
            vt_report_error(0, "Function only accept 2 arguments, but %zu provided", 
                            input_reader.getNumCols());

        const char *argv[4];
        const char *dashc = "-c";
        argv[0] = SHELL_EXECUTABLE;
        argv[1] = dashc;
        argv[2] = NULL; // to be filled in
        argv[3] = NULL; // terminator

        char *envp[1];
        envp[0] = NULL; // no env passed for the moment.  later

        char outputline[LINE_MAX];
        char buf[BUF_SIZE];

        // While we have inputs to process
        do {
            // Get a copy of input id
            const VString &id = input_reader.getStringRef(0);
            if (id.isNull())
            {
                vt_report_error(0, "Cannot supply null for id");
            }
            std::string idstr = id.str();

            // Get a copy of the input command
            const VString &cmd = input_reader.getStringRef(1);
            if (cmd.isNull())
            {
                VString &outcmd = output_writer.getStringRef(0);
                outcmd.setNull();
                VString &outtext = output_writer.getStringRef(1);
                outtext.setNull();
                output_writer.next();
            }
            std::string cmdstr = cmd.str();
            argv[2] = cmdstr.c_str();
            
            ScopedPipes stdinpipe;
            ScopedPipes stdoutpipe;

            pid_t result = vfork();
            if (result == 0)
            {
                // setup pipes to stdin, stdout, stderr
                // cannot use method, as it modifies memory (disallowed under vfork)
                close(0);
                dup(stdinpipe.pipes[0]);
                close(1);
                dup(stdoutpipe.pipes[1]);
                close(2);
                dup(stdoutpipe.pipes[1]);

                if (execve(SHELL_EXECUTABLE,const_cast<char * const *>(argv),envp))
                {
                    _exit(-1); // silently exit
                }
            }

            ScopedSubprocess subproc(result);

            // close far side of pipes
            stdinpipe.closePipe(0);
            stdoutpipe.closePipe(1);
           
            // do not provide any input to command
            stdinpipe.closePipe(0);
            
            // read  output
            int bufend = 0;
            bool hadOutput = false;
            ssize_t count=0;
            // read 1K chunks from pipe
            while ((count = read(stdoutpipe.pipes[0], buf, BUF_SIZE)))
            {
                // iterate, copying into 
                for (ssize_t ptr = 0; ptr < count; ptr++)
                {
                    if (buf[ptr] == '\n')
                    {
                        // ensure null terminated
                        //outputline[bufend] = '\0'; 
                        // Copy string into results
                        output_writer.getStringRef(0).copy(idstr);
                        output_writer.getStringRef(1).copy(cmdstr);
                        output_writer.getStringRef(2).copy(outputline,bufend);
                        output_writer.next();
                        bufend = 0; // reset
                        hadOutput = true;
                    }
                    else if (bufend < LINE_MAX)
                    {
                        outputline[bufend++] = buf[ptr];
                    }
                    // discard characters on too long line
                }
            }
            // results from last line
            if (bufend > 0)
            {
                // ensure null terminated
                //outputline[bufend] = '\0'; 
                // Copy string into results
                output_writer.getStringRef(0).copy(idstr);
                output_writer.getStringRef(1).copy(cmdstr);
                output_writer.getStringRef(2).copy(outputline);
                output_writer.next();
                bufend = 0; // reset
            } 
            else if (!hadOutput) // force at least 1 row of output
            {
                output_writer.getStringRef(0).copy(idstr);
                output_writer.getStringRef(1).copy(cmdstr);
                output_writer.getStringRef(2).setNull();
                output_writer.next();
            }

            subproc.terminate(false/*gently!*/);
        } while (input_reader.next());
    }
};

class ShellFactory : public TransformFunctionFactory
{
    // Tell Vertica that we take in a row with 1 string, and return a row with 2 strings
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        argTypes.addVarchar();

        returnType.addVarchar();
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
        if (input_types.getColumnCount() != 2)
            vt_report_error(0, "Function only accepts 2 arguments, but %zu provided", input_types.getColumnCount());

        // first column outputs the id string passed in
        int input_len = input_types.getColumnType(0).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "id");

        // second column outputs the shell command string used to generate output
        input_len = input_types.getColumnType(1).getStringLength();

        // Our output size will never be more than the input size
        output_types.addVarchar(input_len, "command");

        // other output is a line of output from the shell command, which is truncated at 65000 characters
        output_types.addVarchar(65000, "text");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, Shell); }

};

RegisterFactory(ShellFactory);

