#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Vertica.h"

using namespace Vertica;

/*
 *Send an email
 */
class Email : public ScalarFunction {
	public:
		/*
		 * This method processes a block of rows in a single invocation
		 * 
		 *The inputs are retrieved via arg_reader
		 *The outputs are returned via arg_writer
		 */
		virtual void processBlock(ServerInterface &srvInterface, BlockReader &arg_reader, BlockWriter &res_writer){

			//Basic error checking
			do{
				if (arg_reader.getNumCols() != 4)//NOTE: I need to decide what the inputs are
					vt_report_error(0, "Function only accepts 4 arguments, but %zu provided", arg_reader.getNumCols());
				
				const std::string subject = arg_reader.getStringRef(1).str();
				const bool isFile = arg_reader.getBoolRef(2);
				std::string mFile = arg_reader.getStringRef(3).str();

				if(sizeof(mFile) > 500)
					vt_report_error(0, "Function only supports short inline messages.");

				const std::string & address = arg_reader.getStringRef(0).str();
				
				fflush(stdout);
				fflush(stdin);
				fflush(stderr);

				int pid, status;
				switch((pid = fork())){
				case -1:
					srvInterface.log("processBlock: -1 in fork");
					perror("fork");
					break;
				case 0:
					sendEmail(address, subject, mFile, isFile, srvInterface);
					break;
				default:
					pid = wait(&status);
				}

				res_writer.getStringRef().copy("Ok");
				res_writer.next();
			}while(arg_reader.next());
			srvInterface.log("Done!"); 
		}

	private:

		//Does not return because of crashing process
		void sendEmail(const std::string& address, const std::string& subject, const std::string& mFile, const bool isFile, ServerInterface &srvInterface){
			int pipefd[2];

			//Get a pipe
			if (pipe(pipefd)){
				srvInterface.log("sendEmail: if(pipe(pipefd)) error");
				perror("pipe");
				exit(127);
			}

			//Need to fork again because we need to dispose of two processes
			switch(fork()){
			case -1:
				srvInterface.log("sendEmail: fork case -1 error");
				perror("fork");
				exit(127);
			case 0://Close wrong side of pipe
				close(pipefd[0]);
				dup2(pipefd[1], 1);
				close(pipefd[1]);
				if(isFile){
					execl("/bin/cat", "cat", mFile.c_str(), (char *)NULL);
				}else{
					//call execl
					execl("/bin/echo", "echo", mFile.c_str(), (char *)NULL);//for some reason here my online resource lists the command twice. Not sure why that is
				}
				//This shouldn't actually return
				srvInterface.log("sendEmail: /bin/echo or cat error");
				perror("/bin/echo");//Once again, srvInterface I think would be the correct thing to put here
				exit(126);
			default:
				switch(fork()){
				case 0:
					close(pipefd[1]);
					dup2(pipefd[0], 0);
					close(pipefd[0]);
					execl("/bin/mail", "mail", "-s", subject.c_str(), address.c_str(), (char *)NULL);
					srvInterface.log("sendEmail: below execl default fork error");
					perror("/bin/mail");
					exit(124); 
				}
				int status;
				wait(&status);
			}
		}
}; 

class EMAILFactory : public ScalarFunctionFactory{
	virtual ScalarFunction * createScalarFunction(ServerInterface &interface){
		return vt_createFuncObj(interface.allocator, Email);
	}

	virtual void getPrototype(ServerInterface &interface, ColumnTypes &argTypes, ColumnTypes &returnType){
		argTypes.addVarchar();
		argTypes.addVarchar();
		argTypes.addBool();
		argTypes.addVarchar();

		returnType.addVarchar();
	}

	virtual void getReturnType(ServerInterface &srvInterface, const SizedColumnTypes &argTypes, SizedColumnTypes &returnType){

		returnType.addVarchar(65000);
	} 
}; 

RegisterFactory(EMAILFactory);
